#include "SpUnit.h"
#include "Net/UnrealNetwork.h"
#include "Components/CapsuleComponent.h"
#include "Interface/SpUnitManageListener.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Component/SpUnitClientGatewayComponent.h"
#include "Component/SpUnitServerGatewayComponent.h"
#include "Component/Common/SpUnitStateComponent.h"
#include "Component/Common/SpUnitStimuliSourceComponent.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/Attribute/SpSpeedAttributeSet.h"
#include "ProjectSP/Common/SpLog.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Subsystem/UnitRegistrySubsystem.h"
#include "ProjectSP/Subsystem/ActorPool/ActorPoolSubsystem.h"

// ==================================================

ASpUnit::ASpUnit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	UnitStateComponent = CreateDefaultSubobject<USpUnitStateComponent>("SpUnitStateComponent");
	UnitStateComponent->SetIsReplicated(true); // [mj] todo)
	
	ServerGateway = CreateDefaultSubobject<USpUnitServerGatewayComponent>("SpUnitServerGateway");
	ClientGateway = CreateDefaultSubobject<USpUnitClientGatewayComponent>("SpUnitClientGateway");

	CreateDefaultSubobject<USpUnitStimuliSourceComponent>("SpUnitStimuliSourceComponent");
}

void ASpUnit::BeginPlay()
{
	Super::BeginPlay();

	if (GetNetMode() != NM_DedicatedServer && ClientGateway)
		ClientGateway->InitializePresentation_ClientOnly();
}

// 풀

void ASpUnit::Push()
{
	UActorPoolSubsystem* ActorPoolSubsystem = UActorPoolSubsystem::Get(GetWorld());
	if (!ActorPoolSubsystem)
		return;

	ActorPoolSubsystem->ReturnActor(this);
}

void ASpUnit::OnCreate()
{
	InitUnit();
}

void ASpUnit::OnDestroy()
{
	ClearUnit();
}

void ASpUnit::OnSpawn()
{
	// ActorPool은 액터 생성 완료와 게임플레이 활성화를 분리한다.
	// GamePhase가 Playing으로 전환되면 USpUnitServerGateway가 이 유닛을 활성화한다.
}

void ASpUnit::OnReturn()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DeadProcessTimerHandle);

	bDeadPresentationStarted = false;
	if (HasAuthority() && ServerGateway)
		ServerGateway->ReturnUnit_ServerOnly();
}

// 유닛 초기화

void ASpUnit::InitUnit()
{
	CacheUnitManageComponents();
	InitUnitManageComponents();
	RegisterUnitTagChangedEvent();
}

void ASpUnit::ClearUnit()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DeadProcessTimerHandle);

	UnregisterUnitTagChangedEvent();
	ClearUnitManageComponents();
}

void ASpUnit::SetUnitActive_ServerOnly(bool bActive)
{
	check(HasAuthority());
	
	if (bActive)
		ResetBattleFlag();
	else
		RemoveReplicatedTag(FSpGameplayTags::Get().BattleFlagTag_Targetable);
	
	if (UUnitRegistrySubsystem* UnitRegistrySubsystem = UUnitRegistrySubsystem::Get(GetWorld()))
	{
		if (bActive)
			UnitRegistrySubsystem->RegisterUnit(this);
		else
			UnitRegistrySubsystem->UnregisterUnit(this);
	}

	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		if (bActive)
		{
			MovementComp->SetMovementMode(MOVE_Walking);
		}
		else
		{
			MovementComp->StopMovementImmediately();
			MovementComp->DisableMovement();
		}
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (ISpUnitManageListener* ControllerListener = Cast<ISpUnitManageListener>(GetController()))
		ControllerListener->OnUnitActive(bActive);

	for (UActorComponent* Component : UnitManageComponents)
	{
		if (ISpUnitManageListener* Listener = Cast<ISpUnitManageListener>(Component))
			Listener->OnUnitActive(bActive);
	}
}

// 상태

void ASpUnit::Dead()
{
	const FGameplayTag DeadTag = FSpGameplayTags::Get().StateTag_Dead;
	if (!GetSpAbilitySystemComponent() || !HasAuthority() || IsActorBeingDestroyed())
		return;

	if (!HasTag(DeadTag, true))
		AddReplicatedTag(DeadTag);
}

void ASpUnit::K2_BeginDeadPresentation()
{
	BeginDeadPresentation();
}

void ASpUnit::K2_FinishDeadPresentation()
{
	FinishDeadPresentation();
}

// 유닛 상세 설정

void ASpUnit::ApplyUnitData_ServerOnly(const FSpUnitData& InUnitData)
{
	check(HasAuthority());
	check(InUnitData.IsValid());

	UnitData = InUnitData;
}

void ASpUnit::ResetUnitData_ServerOnly()
{
	check(HasAuthority());
	
	UnitData.Reset();
	SetGenericTeamId(FGenericTeamId::NoTeam);
}

void ASpUnit::ApplyReplicatedUnitData_ClientOnly()
{
	// 클라이언트 유닛에 해당하는지
	check(!HasAuthority() || GetNetMode() == NM_ListenServer || GetNetMode() == NM_Standalone);
	
	SetGenericTeamId(FGenericTeamId(UnitData.TeamId));
}

// 팀

FGenericTeamId ASpUnit::GetGenericTeamId() const
{
	return UnitData.TeamId;
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

ETeamAttitude::Type ASpUnit::GetTeamAttitudeTowards(const AActor& Other) const
{
	return GetTeamAttitudeTowards(Cast<IGenericTeamAgentInterface>(&Other));
}

// 보유 태그

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

bool ASpUnit::IsTargetable(const ASpUnit* Attacker) const
{
	return Attacker && Attacker != this && HasTag(FSpGameplayTags::Get().BattleFlagTag_Targetable);
}

bool ASpUnit::IsAttackable(const ASpUnit* Attacker) const
{
	return IsTargetable(Attacker) && Attacker->GetTeamAttitudeTowards(this) == ETeamAttitude::Hostile;
}

// 이동

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
	const float RotationEndDegree = FMath::Max(SpeedAttributeSet->GetRotationToleranceDegrees(), 0.0f);
	
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
	
	const float NewYaw = CurrentRotation.Yaw + AppliedDeltaYaw;
	const float RemainingYaw = FMath::Abs(FMath::FindDeltaAngleDegrees(NewYaw, TargetRotation.Yaw));
	return RemainingYaw > RotationEndDegree;
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

// 어빌리티

UAbilitySystemComponent* ASpUnit::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USpAbilitySystemComponent* ASpUnit::GetSpAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASpUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpUnit, UnitData);
}

// 내부 구현 ------------------------------------------------

// 컴포넌트 설정

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

// 유닛 상세 설정

void ASpUnit::ResetBattleFlag()
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (!ASC || !HasAuthority())
		return;

	// 상태 태그를 일괄 삭제한다.
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	const FGameplayTag StateRoot = FSpGameplayTags::Get().StateTag;
	for (const FGameplayTag& Tag : OwnedTags)
	{
		if (Tag.MatchesTag(StateRoot))
			ASC->SetLooseGameplayTagCount(Tag, 0, EGameplayTagReplicationState::TagOnly);
	}
	
	// 대상 지정 가능 상태를 처리한다.
	AddReplicatedTag(FSpGameplayTags::Get().BattleFlagTag_Targetable);
}

// 태그 이벤트

void ASpUnit::RegisterUnitTagChangedEvent()
{
	USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent();
	if (UnitTagChangedEventHandle.IsValid() || !ASC)
		return;

	UnitTagChangedEventHandle = ASC->RegisterGenericGameplayTagEvent().AddUObject(this, &ThisClass::HandleUnitTagChanged);
}

void ASpUnit::UnregisterUnitTagChangedEvent()
{
	if (!UnitTagChangedEventHandle.IsValid())
		return;

	if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
		ASC->RegisterGenericGameplayTagEvent().Remove(UnitTagChangedEventHandle);

	UnitTagChangedEventHandle.Reset();
}

void ASpUnit::HandleUnitTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!Tag.MatchesTag(FSpGameplayTags::Get().StateTag))
		return;

	const bool bAdded = NewCount > 0;

	HandleStateChanged(Tag, bAdded);
	NotifyUnitStateChanged(Tag, bAdded);
}

// 사망

void ASpUnit::BeginDeadPresentation()
{
	if (bDeadPresentationStarted || IsActorBeingDestroyed())
		return;

	bDeadPresentationStarted = true;

	// C++ 처리
	StartDeadProcess();
}

void ASpUnit::FinishDeadPresentation()
{
	if (!bDeadPresentationStarted || IsActorBeingDestroyed())
		return;

	bDeadPresentationStarted = false;

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		if (TimerManager.IsTimerActive(DeadProcessTimerHandle) || TimerManager.IsTimerPaused(DeadProcessTimerHandle))
			return;

		const float DeadProcessTime = UnitData.UnitDefinition ? UnitData.UnitDefinition->DeadProcessTime : 0.0f;
		if (DeadProcessTime > 0.0f)
		{
			TimerManager.SetTimer(DeadProcessTimerHandle, this, &ThisClass::FinishDeadProcess, DeadProcessTime, false);
			return;
		}
	}

	// C++ 처리
	FinishDeadProcess();
}

void ASpUnit::StartDeadProcess()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
		return;

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DeadProcessTimerHandle);

	SetUnitActive_ServerOnly(false);
}

void ASpUnit::FinishDeadProcess()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
		return;

	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DeadProcessTimerHandle);

	EUnitDeadProcess Type = UnitData.UnitDefinition ? UnitData.UnitDefinition->DeadProcess : EUnitDeadProcess::Remove;
	switch (Type)
	{
		case EUnitDeadProcess::Resurrect:
		{
			ResurrectByDeadProcess();
			break;
		}
		case EUnitDeadProcess::Remove: default:
		{
			DestroyByDeadProcess();
			break;
		}
	}
}

void ASpUnit::DestroyByDeadProcess()
{
	Push();
}

void ASpUnit::ResurrectByDeadProcess()
{
	if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
		ASC->RestoreHp();

	SetUnitActive_ServerOnly(true);
}

// 상태

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
	if (!StateTag.MatchesTagExact(FSpGameplayTags::Get().StateTag_Dead))
		return;

	if (!bAdded)
		return;

	BeginDeadPresentation();
}

// 복제

void ASpUnit::OnRep_UnitData()
{
	if (ClientGateway)
		ClientGateway->ApplyUnitData_ClientOnly(UnitData);
}
