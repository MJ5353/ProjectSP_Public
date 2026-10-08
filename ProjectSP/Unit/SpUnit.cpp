#include "SpUnit.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Interface/SpUnitManageListener.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Component/SpUnitClientGatewayComponent.h"
#include "Component/SpUnitServerGatewayComponent.h"
#include "Component/Common/SpUnitHPBarComponent.h"
#include "Component/Common/SpUnitStateComponent.h"
#include "Component/Common/SpUnitStimuliSourceComponent.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Attribute/SpSpeedAttributeSet.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Subsystem/UnitRegistrySubsystem.h"

// ==================================================

ASpUnit::ASpUnit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<USpAbilitySystemComponent>(TEXT("SpAbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	UnitStateComponent = CreateDefaultSubobject<USpUnitStateComponent>("SpUnitStateComponent");
	UnitStateComponent->SetIsReplicated(true);
	
	ServerGateway = CreateDefaultSubobject<USpUnitServerGatewayComponent>("SpUnitServerGateway");
	ClientGateway = CreateDefaultSubobject<USpUnitClientGatewayComponent>("SpUnitClientGateway");

	CreateDefaultSubobject<USpUnitStimuliSourceComponent>("SpUnitStimuliSourceComponent");
}

void ASpUnit::BeginPlay()
{
	Super::BeginPlay();

	// 서버와 클라이언트가 동일한 유닛 초기화 경로를 사용한다.
	InitUnit();

	if (GetNetMode() != NM_DedicatedServer && ClientGateway)
		ClientGateway->InitializePresentation_Client();
}

void ASpUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearUnit();
	
	Super::EndPlay(EndPlayReason);
}

// Core ------------------------------------------------

void ASpUnit::InitUnit()
{
	if (!bUnitManageComponentsInitialized)
	{
		CacheUnitManageComponents();
		InitUnitManageComponents();
		
		bUnitManageComponentsInitialized = true;
	}

	if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
		ASC->InitAbilityActorInfo(this, this);
	
	RegisterUnitTagChangedEvent();
}

void ASpUnit::ClearUnit()
{
	if (HasAuthority())
		OnSourceUnavailable.Broadcast();

	InvalidateTarget();

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DeadProcessTimerHandle);

	UnregisterUnitTagChangedEvent();

	if (bUnitManageComponentsInitialized)
	{
		ClearUnitManageComponents();
		bUnitManageComponentsInitialized = false;
	}
}

void ASpUnit::SetUnitPlayable(bool bPlayable)
{
	SetActorEnableCollision(bPlayable);

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		ECollisionEnabled::Type CollisionType = bPlayable ? ECollisionEnabled::Type::QueryOnly : ECollisionEnabled::Type::NoCollision;
		Capsule->SetCollisionEnabled(CollisionType);
	}

	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		if (bPlayable)
		{
			MovementComp->SetMovementMode(MOVE_Walking);
		}
		else
		{
			MovementComp->StopMovementImmediately();
			MovementComp->DisableMovement();
		}
	}

	if (ISpUnitManageListener* ControllerListener = Cast<ISpUnitManageListener>(GetController()))
	{
		ControllerListener->OnUnitPlayable(bPlayable);
	}

	for (UActorComponent* Component : UnitManageComponents)
	{
		if (ISpUnitManageListener* Listener = Cast<ISpUnitManageListener>(Component))
			Listener->OnUnitPlayable(bPlayable);
	}

	if (HasAuthority())
	{
		if (bPlayable)
			ResetBattleFlag();
		else
			RemoveReplicatedTag(SpGameplayTags::UnitFlagTag_Targetable);
		
		if (UUnitRegistrySubsystem* UnitRegistrySubsystem = UUnitRegistrySubsystem::Get(GetWorld()))
		{
			if (bPlayable)
				UnitRegistrySubsystem->RegisterUnit(this);
			else
				UnitRegistrySubsystem->UnregisterUnit(this);
		}
		
		if (bPlayable)
		{
			USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
			const USpUnitDefinition* UnitDefinition = GetUnitDefinition();

			if (ASC && UnitDefinition && UnitDefinition->AttributeDefinition)
				UnitDefinition->AttributeDefinition->ResetAttributes(ASC);
		}
	}

	if (!bPlayable)
		InvalidateTarget();
}

void ASpUnit::ResetBattleFlag()
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!ASC || !HasAuthority())
		return;

	// 상태 태그를 일괄 삭제한다.
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const FGameplayTag StateRoot = SpGameplayTags::UnitStateTag;
	for (const FGameplayTag& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(StateRoot))
			ASC->SetLooseGameplayTagCount(Tag, 0, EGameplayTagReplicationState::TagOnly);
	}
	
	// 대상 지정 가능 상태를 처리한다.
	AddReplicatedTag(SpGameplayTags::UnitFlagTag_Targetable);
}

void ASpUnit::SetUnitActive_Server(bool bActive, bool bSetPlayable)
{
	check(HasAuthority());
	if (!bActive)
		OnSourceUnavailable.Broadcast();

	SetActorHiddenInGame(!bActive);
	SetActorTickEnabled(bActive);

	if (ISpUnitManageListener* ControllerListener = Cast<ISpUnitManageListener>(GetController()))
		ControllerListener->OnUnitActive(bActive);

	for (UActorComponent* Component : UnitManageComponents)
	{
		if (ISpUnitManageListener* Listener = Cast<ISpUnitManageListener>(Component))
			Listener->OnUnitActive(bActive);
	}

	if (bSetPlayable)
		SetUnitPlayable(bActive);
}

void ASpUnit::SetUnitPresentationVisible_Client(const bool bVisible)
{
	check(GetNetMode() != NM_DedicatedServer);
	SetActorHiddenInGame(!bVisible);
}

void ASpUnit::SetUnitPresentationVisible_Server(const bool bVisible)
{
	check(HasAuthority());
	SetActorHiddenInGame(!bVisible);
}

// Comp

void ASpUnit::CacheUnitManageComponents()
{
	TArray<UActorComponent*> Components;
	GetComponents(Components);

	TArray<UActorComponent*> ListenerComponents;
	ListenerComponents.Reserve(Components.Num());
	for (UActorComponent* Component : Components)
	{
		if (IsValid(Component) && Cast<ISpUnitManageListener>(Component))
			ListenerComponents.Add(Component);
	}

	ListenerComponents.Sort([](const UActorComponent& A, const UActorComponent& B)
	{
		const ISpUnitManageListener* ListenerA = Cast<ISpUnitManageListener>(&A);
		const ISpUnitManageListener* ListenerB = Cast<ISpUnitManageListener>(&B);
		const int32 PriorityA = ListenerA ? ListenerA->GetUnitManagePriority() : 0;
		const int32 PriorityB = ListenerB ? ListenerB->GetUnitManagePriority() : 0;
		return PriorityA < PriorityB;
	});

	UnitManageComponents.Reset(ListenerComponents.Num());
	
	for (UActorComponent* Component : ListenerComponents)
		UnitManageComponents.Add(Component);
}

void ASpUnit::InitUnitManageComponents()
{
	for (UActorComponent* Component : UnitManageComponents)
	{
		if (ISpUnitManageListener* Listener = Cast<ISpUnitManageListener>(Component))
			Listener->OnInitUnit();
	}
}

void ASpUnit::ClearUnitManageComponents()
{
	for (UActorComponent* Component : UnitManageComponents)
	{
		if (ISpUnitManageListener* Listener = Cast<ISpUnitManageListener>(Component))
			Listener->OnClearUnit();
	}

	UnitManageComponents.Reset();
}

// Unit Data 

void ASpUnit::ApplyUnitData_Server(const FSpUnitData& InUnitData)
{
	check(HasAuthority());
	check(InUnitData.IsValid());

	UnitData = InUnitData;
}

void ASpUnit::ResetUnitData_Server()
{
	check(HasAuthority());
	OnSourceUnavailable.Broadcast();
	InvalidateTarget();
	
	UnitData.Reset();
	SetGenericTeamId(FGenericTeamId::NoTeam);
}

void ASpUnit::ApplyReplicatedUnitData_Client()
{
	// Listen Server 호스트와 Standalone도 로컬 표현 데이터를 적용한다.
	check(!HasAuthority() || GetNetMode() == NM_ListenServer || GetNetMode() == NM_Standalone);
	
	SetGenericTeamId(FGenericTeamId(UnitData.TeamId));
}

void ASpUnit::OnRep_UnitData()
{
	if (!HasAuthority())
	{
		if (UnitData.IsValid())
			InitUnit();
		else
			ClearUnit();
	}

	if (ClientGateway)
		ClientGateway->ApplyUnitData_Client(UnitData);
}

// Dead ------------------------------------------------

void ASpUnit::Dead()
{
	const FGameplayTag DeadTag = SpGameplayTags::UnitStateTag_Dead;
	if (!GetSpAbilitySystemComponent() || !HasAuthority() || IsActorBeingDestroyed())
		return;

	if (!CheckDead())
		AddReplicatedTag(DeadTag);
}

void ASpUnit::BeginDead()
{
	if (IsActorBeingDestroyed())
		return;

	if (HasAuthority())
		StartDeadProcess_Server();
	
	if (GetNetMode() != NM_DedicatedServer)
		StartDeadPresentation_Client();
}

void ASpUnit::StartDeadProcess_Server()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
		return;
	
	if (GetWorldTimerManager().IsTimerActive(DeadProcessTimerHandle) || !CheckDead())
		return;

	SetUnitPlayable(false);

	const float DeadProcessTime = UnitData.UnitDefinition ? UnitData.UnitDefinition->DeadProcessTime : 0.0f;
	const float Delay = FMath::Max(DeadProcessTime, 0.0f);
	
	if (Delay <= 0.0f)
	{
		FinishDeadProcess_Server();
		return;
	}
	
	GetWorldTimerManager().SetTimer(DeadProcessTimerHandle, this, &ThisClass::FinishDeadProcess_Server, Delay, false);
}

void ASpUnit::FinishDeadProcess_Server()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
		return;

	GetWorldTimerManager().ClearTimer(DeadProcessTimerHandle);

	// 타이머가 도는 사이 부활·반환됐다면 오래된 타이머는 무시
	if (!CheckDead())
		return;

	HandleDeadProcessFinished_Server();
}

void ASpUnit::StartDeadPresentation_Client()
{
	if (GetNetMode() == NM_DedicatedServer)
		return;
	
	if (bDeadPresentationActive || IsActorBeingDestroyed())
		return;
	
	if (!CheckDead())
		return;
	
	SetUnitPlayable(false);
	
	bDeadPresentationActive = true;
	K2_BeginDeadPresentation();
}

void ASpUnit::FinishDeadPresentation_Client()
{
	if (!bDeadPresentationActive)
		return;
	
	SetUnitPlayable(true);
	SetUnitPresentationVisible_Client(true);
	
	bDeadPresentationActive = false;
	K2_FinishDeadPresentation();

	if (USpUnitHPBarComponent* HPBar = GetUnitComponent<USpUnitHPBarComponent>())
		HPBar->RefreshPresentation_Client();
}

void ASpUnit::HandleDeadProcessFinished_Server()
{
	check(HasAuthority());
	
	Destroy();
}

// Tag ------------------------------------------------

void ASpUnit::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.Reset();
	if (const USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
		ASC->GetOwnedGameplayTags(TagContainer);
}

void ASpUnit::AddLocalTag(FGameplayTag Tag)
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!Tag.IsValid() || !ASC || HasTag(Tag, true))
		return;

	ASC->SetLooseGameplayTagCount(Tag, 1, EGameplayTagReplicationState::None);
}

void ASpUnit::RemoveLocalTag(FGameplayTag Tag)
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!Tag.IsValid() || !ASC || !HasTag(Tag, true))
		return;

	ASC->SetLooseGameplayTagCount(Tag, 0, EGameplayTagReplicationState::None);
}

void ASpUnit::AddReplicatedTag(FGameplayTag Tag)
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!Tag.IsValid() || !ASC || !HasAuthority() || HasTag(Tag, true))
		return;

	ASC->SetLooseGameplayTagCount(Tag, 1, EGameplayTagReplicationState::TagOnly);
}

void ASpUnit::RemoveReplicatedTag(FGameplayTag Tag)
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!Tag.IsValid() || !ASC || !HasAuthority() || !HasTag(Tag, true))
		return;

	ASC->SetLooseGameplayTagCount(Tag, 0, EGameplayTagReplicationState::TagOnly);
}

bool ASpUnit::HasTag(FGameplayTag Tag, bool bExact) const
{
	const USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!Tag.IsValid() || !ASC)
		return false;

	if (!bExact)
		return ASC->HasMatchingGameplayTag(Tag);

	FGameplayTagContainer TagContainer;
	ASC->GetOwnedGameplayTags(TagContainer);
	return TagContainer.HasTagExact(Tag);
}

// tag event

void ASpUnit::RegisterUnitTagChangedEvent()
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();

	if (UnitTagChangedEventASC.Get() != ASC)
		UnregisterUnitTagChangedEvent();

	if (!ASC || UnitTagChangedEventHandle.IsValid())
		return;

	UnitTagChangedEventASC = ASC;
	UnitTagChangedEventHandle = ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &ThisClass::HandleUnitTagChanged);
	
	SyncUnitStatesFromAbilitySystem(ASC);
}

void ASpUnit::UnregisterUnitTagChangedEvent()
{
	ResetAppliedUnitStates();

	if (UnitTagChangedEventHandle.IsValid())
	{
		if (USpAbilitySystemComponent* ASC = UnitTagChangedEventASC.Get())
			ASC->RegisterGenericGameplayTagEvent().Remove(UnitTagChangedEventHandle);
	}

	UnitTagChangedEventHandle.Reset();
	UnitTagChangedEventASC.Reset();
}

void ASpUnit::SyncUnitStatesFromAbilitySystem(USpAbilitySystemComponent* ASC)
{
	if (!ASC)
		return;

	const FGameplayTagContainer PreviousStateTags = AppliedUnitStateTags;
	for (const FGameplayTag& Tag : PreviousStateTags)
	{
		// tag가 지워진 경우
		if (ASC->GetTagCount(Tag) <= 0)
			HandleUnitTagChanged(Tag, 0);
	}

	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const FGameplayTag StateRootTag = SpGameplayTags::UnitStateTag;
	for (const FGameplayTag& Tag : OwnedTags)
	{
		// 추가된 tag를 noti하기 위함
		if (Tag.MatchesTag(StateRootTag))
			HandleUnitTagChanged(Tag, ASC->GetTagCount(Tag));
	}
}

void ASpUnit::ResetAppliedUnitStates()
{
	// 모든 tag 날리기
	
	const FGameplayTagContainer PreviousStateTags = AppliedUnitStateTags;
	for (const FGameplayTag& Tag : PreviousStateTags)
	{
		HandleUnitTagChanged(Tag, 0);
	}

	AppliedUnitStateTags.Reset();
}

void ASpUnit::HandleUnitTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!Tag.MatchesTag(SpGameplayTags::UnitStateTag))
		return;

	const bool bAdded = NewCount > 0;
	const bool bWasAdded = AppliedUnitStateTags.HasTagExact(Tag);
	
	if (bAdded == bWasAdded)
		return;
	
	if (bAdded)
		AppliedUnitStateTags.AddTag(Tag);
	else
		AppliedUnitStateTags.RemoveTag(Tag);
	
	HandleStateChanged(Tag, bAdded);
	NotifyUnitStateChanged(Tag, bAdded);
}

// Target ---------------------------------------------

void ASpUnit::SetTargetActor(ASpUnit* InTargetActor)
{
	if (!IsValid(InTargetActor))
		InTargetActor = nullptr;

	if (TargetActor.Get() == InTargetActor && (InTargetActor || !TargetInvalidatedHandle.IsValid()))
		return;

	if (ASpUnit* PreviousTarget = TargetActor.Get())
		PreviousTarget->OnTargetInvalidated.Remove(TargetInvalidatedHandle);

	TargetInvalidatedHandle.Reset();
	TargetActor = InTargetActor;

	if (InTargetActor)
		TargetInvalidatedHandle = InTargetActor->OnTargetInvalidated.AddUObject(this, &ThisClass::HandleTargetInvalidated);

	OnTargetChanged.Broadcast();
}

void ASpUnit::InvalidateTarget()
{
	SetTargetActor(nullptr);
	OnTargetInvalidated.Broadcast();
}

void ASpUnit::HandleTargetInvalidated()
{
	SetTargetActor(nullptr);
}

// Movement ------------------------------------------

bool ASpUnit::MoveToTargetLocation(const FVector& TargetLocation, const float MoveEndDistance, const float SpeedScale)
{
	const float DistanceSq = FVector::DistSquared2D(GetActorLocation(), TargetLocation);
	if (DistanceSq <= FMath::Square(MoveEndDistance))
		return false;

	const FVector Direction = (TargetLocation - GetActorLocation()).GetSafeNormal2D();
	AddMovementInput(Direction, FMath::Clamp(SpeedScale, 0.0f, 1.0f));

	return true;
}

bool ASpUnit::RotateToTargetLocation(const FVector& TargetLocation, float DeltaTime, const float SpeedScale)
{
	const USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	const USpSpeedAttributeSet* SpeedAttributeSet = ASC ? ASC->GetSet<USpSpeedAttributeSet>() : nullptr;
	
	if (!ensure(SpeedAttributeSet))
		return false;

	const float RotationRateDegreesPerSecond = FMath::Max(SpeedAttributeSet->GetRotationRateDegreesPerSecond(), 0.0f);
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.0f;

	if (ToTarget.IsNearlyZero())
		return false;

	const FRotator CurrentRotation(0.0f, GetActorRotation().Yaw, 0.0f);
	const FRotator TargetRotation(0.0f, ToTarget.Rotation().Yaw, 0.0f);

	const float DegreesPerSecond = RotationRateDegreesPerSecond * FMath::Max(SpeedScale, 0.0f);
	const float RemainingDeltaYaw = FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, TargetRotation.Yaw);
	const float MaxDeltaYaw = DegreesPerSecond * FMath::Max(DeltaTime, 0.0f);
	const float AppliedDeltaYaw = FMath::Clamp(RemainingDeltaYaw, -MaxDeltaYaw, MaxDeltaYaw);
	const FRotator RotationDelta(0.0f, AppliedDeltaYaw, 0.0f);
	
	AddActorWorldRotation(RotationDelta);
	return !IsFacingTargetLocation(TargetLocation);
}

bool ASpUnit::IsFacingTargetLocation(const FVector& TargetLocation) const
{
	FVector ToTarget = TargetLocation - GetActorLocation();
	ToTarget.Z = 0.0f;
	if (ToTarget.IsNearlyZero())
		return true;

	const USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	const USpSpeedAttributeSet* SpeedAttributeSet = ASC ? ASC->GetSet<USpSpeedAttributeSet>() : nullptr;
	if (!SpeedAttributeSet)
		return false;

	const float ToleranceDegrees = FMath::Max(SpeedAttributeSet->GetRotationToleranceDegrees(), 0.0f);
	const float TargetYaw = ToTarget.Rotation().Yaw;
	const float CurrentYaw = GetActorRotation().Yaw;
	return FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw)) <= ToleranceDegrees;
}

void ASpUnit::ApplyRotationRateFromAttribute()
{
	const USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	const USpSpeedAttributeSet* SpeedAttributeSet = ASC ? ASC->GetSet<USpSpeedAttributeSet>() : nullptr;
	if (!SpeedAttributeSet)
		return;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
		MovementComponent->RotationRate.Yaw = FMath::Max(SpeedAttributeSet->GetRotationRateDegreesPerSecond(), 0.0f);
}

// State ----------------------------------------------

void ASpUnit::NotifyUnitStateChanged(FGameplayTag StateTag, bool bAdded)
{
	for (UActorComponent* Component : UnitManageComponents)
	{
		if (ISpUnitManageListener* Listener = Cast<ISpUnitManageListener>(Component))
			Listener->OnUnitStateChanged(StateTag, bAdded);
	}

	if (ISpUnitManageListener* ControllerListener = Cast<ISpUnitManageListener>(GetController()))
		ControllerListener->OnUnitStateChanged(StateTag, bAdded);
}

void ASpUnit::HandleStateChanged(FGameplayTag StateTag, bool bAdded)
{
	if (StateTag.MatchesTagExact(SpGameplayTags::UnitStateTag_Dead))
	{
		if (bAdded)
			OnDead();
		else if (GetNetMode() != NM_DedicatedServer)
			FinishDeadPresentation_Client();
	}
}

void ASpUnit::OnDead()
{
	if (HasAuthority())
		OnSourceUnavailable.Broadcast();

	InvalidateTarget();

	if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
		ASC->ResetForDead();
	
	BeginDead();
}

// Get ------------------------------------------------

bool ASpUnit::CheckDead() const
{
	return HasTag(SpGameplayTags::UnitStateTag_Dead, true);
}

bool ASpUnit::IsAttackable(const ASpUnit* Target) const
{
	const FGameplayTag TargetableTag = SpGameplayTags::UnitFlagTag_Targetable;
	const bool bValidAttacker = !CheckDead();
	const bool bValidTarget = Target && Target != this && !Target->CheckDead() && Target->HasTag(TargetableTag, false);
	const bool bHostile = bValidTarget && GetTeamAttitudeTowards(Target) == ETeamAttitude::Hostile;
	
	return bValidAttacker && bValidTarget && bHostile;
}

UAbilitySystemComponent* ASpUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASpUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpUnit, UnitData);
}

// team

FGenericTeamId ASpUnit::GetGenericTeamId() const
{
	return UnitData.TeamId;
}

ETeamAttitude::Type ASpUnit::GetTeamAttitudeTowards(const AActor& Other) const
{
	return GetTeamAttitudeTowards(Cast<IGenericTeamAgentInterface>(&Other));
}

ETeamAttitude::Type ASpUnit::GetTeamAttitudeTowards(const IGenericTeamAgentInterface* OtherTeamAgent) const
{
	if (!OtherTeamAgent)
		return ETeamAttitude::Neutral;

	const uint8 MyTeamId = GetGenericTeamId().GetId();
	const uint8 OtherTeamId = OtherTeamAgent->GetGenericTeamId().GetId();
	const uint8 NoTeamId = FGenericTeamId::NoTeam.GetId();

	if (MyTeamId == NoTeamId || OtherTeamId == NoTeamId)
		return ETeamAttitude::Neutral;

	return MyTeamId == OtherTeamId ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}
