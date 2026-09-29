#include "SpGameplayField.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "ProjectSP/Ability/Targeting/SpTargetCollectorLibrary.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

ASpGameplayField::ASpGameplayField()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AActor::SetReplicateMovement(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ASpGameplayField::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearFieldTimers();
	RemoveEffectsOnFinishIfConfigured();
	UnbindSourceUnavailable();

	CurrentTargets.Reset();
	EffectSpecs.Reset();

	Super::EndPlay(EndPlayReason);
}

void ASpGameplayField::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpGameplayField, TargetQuery);
	DOREPLIFETIME(ASpGameplayField, bFieldActive);
	DOREPLIFETIME(ASpGameplayField, VisualMeshRelativeZ);
}

// core

ASpGameplayField* ASpGameplayField::SpawnGameplayField(TSubclassOf<ASpGameplayField> FieldClass, ASpUnit* InSourceUnit, const FSpTargetQuery& InTargetQuery, const TArray<FGameplayEffectSpecHandle>& InEffectSpecs)
{
	const bool bValidClass = FieldClass && !FieldClass->HasAnyClassFlags(CLASS_Abstract);
	const bool bValidSource = CanUseSourceForSpawn(InSourceUnit);
	
	if (!bValidClass || !bValidSource || !InTargetQuery.IsValid())
		return nullptr;

	UWorld* World = InSourceUnit->GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
		return nullptr;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = InSourceUnit;
	SpawnParams.Instigator = Cast<APawn>(InSourceUnit);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpGameplayField* Field = World->SpawnActor<ASpGameplayField>(FieldClass, InTargetQuery.Origin, FRotator::ZeroRotator, SpawnParams);
	if (!IsSpawnedFieldAvailable(Field))
		return nullptr;

	const uint32 InitialSourceUid = InSourceUnit->GetUnitUid();
	const bool bSourceStillAvailable = CanUseSourceForSpawn(InSourceUnit);
	const bool bSameSourceUid = bSourceStillAvailable && InSourceUnit->GetUnitUid() == InitialSourceUid;
	
	if (!bSourceStillAvailable || !bSameSourceUid)
	{
		Field->ReleaseFailedGameplayField();
		return nullptr;
	}
	
	Field->SetVisualMeshGroundOffset(InSourceUnit, InTargetQuery.Origin);

	// init
	if (!Field->InitializeGameplayField(InSourceUnit, InTargetQuery, InEffectSpecs))
	{
		Field->ReleaseFailedGameplayField();
		return nullptr;
	}

	return Field;
}

ASpGameplayField* ASpGameplayField::SpawnGameplayField_CustomConfig(TSubclassOf<ASpGameplayField> FieldClass, ASpUnit* InSourceUnit, const FSpGameplayFieldConfig& InFieldConfig, const FSpTargetQuery& InTargetQuery, const TArray<FGameplayEffectSpecHandle>& InEffectSpecs)
{
	const bool bValidClass = FieldClass && !FieldClass->HasAnyClassFlags(CLASS_Abstract);
	const bool bValidSource = CanUseSourceForSpawn(InSourceUnit);
	
	if (!bValidClass || !bValidSource || !InTargetQuery.IsValid())
		return nullptr;

	UWorld* World = InSourceUnit->GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
		return nullptr;
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = InSourceUnit;
	SpawnParams.Instigator = Cast<APawn>(InSourceUnit);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ASpGameplayField* Field = World->SpawnActor<ASpGameplayField>(FieldClass, InTargetQuery.Origin, FRotator::ZeroRotator, SpawnParams);
	if (!IsSpawnedFieldAvailable(Field))
		return nullptr;

	const uint32 InitialSourceUid = InSourceUnit->GetUnitUid();
	const bool bSourceStillAvailable = CanUseSourceForSpawn(InSourceUnit);
	const bool bSameSourceUid = bSourceStillAvailable && InSourceUnit->GetUnitUid() == InitialSourceUid;
	
	if (!bSourceStillAvailable || !bSameSourceUid)
	{
		Field->ReleaseFailedGameplayField();
		return nullptr;
	}
	
	Field->SetVisualMeshGroundOffset(InSourceUnit, InTargetQuery.Origin);

	// init
	Field->FieldConfig = InFieldConfig;
	if (!Field->InitializeGameplayField(InSourceUnit, InTargetQuery, InEffectSpecs))
	{
		Field->ReleaseFailedGameplayField();
		return nullptr;
	}

	return Field;
}

void ASpGameplayField::EvaluateGameplayField()
{
	if (!CheckCanContinueField())
		return;

	RefreshBoundTargetQuery();

	TArray<ASpUnit*> CollectedTargets;
	USpTargetCollectorLibrary::CollectTargetsForField(GetWorld(), SourceUnitUid, SourceTeamId, TargetQuery, CollectedTargets);

	ProcessCollectedTargets(CollectedTargets);
}

void ASpGameplayField::FinishGameplayField()
{
	if (!HasAuthority() || !bFieldActive)
		return;

	bFieldActive = false;
	ClearFieldTimers();

	const TSet<TWeakObjectPtr<ASpUnit>> NoRemainingTargets;
	NotifyExitedTargets(NoRemainingTargets);

	RemoveEffectsOnFinishIfConfigured();

	CurrentTargets.Reset();

	K2_OnGameplayFieldFinished();
	ForceNetUpdate();
	ReleaseGameplayField();
}

void ASpGameplayField::DestroyGameplayField()
{
	if (!HasAuthority() || IsActorBeingDestroyed())
		return;

	if (bFieldActive)
		FinishGameplayField();
	else
		ReleaseGameplayField();
}

void ASpGameplayField::ReleaseGameplayField()
{
	Destroy();
}

bool ASpGameplayField::ApplyFieldEffectsToTarget_Implementation(ASpUnit* Target)
{
	if (!HasAuthority() || !IsValid(Target))
		return false;

	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();

	if (!TargetASC)
		return false;

	bool bAppliedAnyEffect = false;
	const bool bTrackAppliedEffects = FieldConfig.bRemoveAppliedEffectsOnExit || FieldConfig.bRemoveAppliedEffectsOnFinish;
	for (const FGameplayEffectSpecHandle& EffectSpec : EffectSpecs)
	{
		if (!EffectSpec.IsValid())
			continue;

		const FActiveGameplayEffectHandle AppliedHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
		if (!AppliedHandle.WasSuccessfullyApplied())
			continue;

		bAppliedAnyEffect = true;

		if (AppliedHandle.IsValid() && bTrackAppliedEffects)
			AppliedEffectHandles.FindOrAdd(TWeakObjectPtr<ASpUnit>(Target)).Add(AppliedHandle);
	}

	return bAppliedAnyEffect;
}

// static

bool ASpGameplayField::CanUseSourceForSpawn(const ASpUnit* InSourceUnit)
{
	if (!IsValid(InSourceUnit))
		return false;

	const bool bUsableActor = InSourceUnit->HasAuthority() && !InSourceUnit->IsActorBeingDestroyed();
	const bool bUsableUnit = InSourceUnit->GetUnitUid() != FSpUnitData::InvalidUnitUid && !InSourceUnit->CheckDead();
	return bUsableActor && bUsableUnit;
}

bool ASpGameplayField::IsSpawnedFieldAvailable(const ASpGameplayField* Field)
{
	return IsValid(Field) && !Field->IsActorBeingDestroyed();
}

void ASpGameplayField::RemoveEffectHandlesFromTarget(ASpUnit* Target, const TArray<FActiveGameplayEffectHandle>& Handles)
{
	UAbilitySystemComponent* TargetASC = IsValid(Target) ? Target->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
		return;

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		TargetASC->RemoveActiveGameplayEffect(Handle);
	}
}

// init

bool ASpGameplayField::InitializeGameplayField(ASpUnit* InSourceUnit, const FSpTargetQuery& InTargetQuery, const TArray<FGameplayEffectSpecHandle>& InEffectSpecs)
{
	const bool bValidFieldState = HasAuthority() && !bFieldActive && !IsActorBeingDestroyed();
	if (!bValidFieldState)
		return false;
	
	const bool bValidSource = IsValid(InSourceUnit) && InSourceUnit->GetWorld() == GetWorld() && InSourceUnit->GetUnitUid() != FSpUnitData::InvalidUnitUid;
	if (!bValidSource || !InTargetQuery.IsValid())
		return false;
	
	const bool bValidDuration = FMath::IsFinite(FieldConfig.Duration) && FieldConfig.Duration > 0.0f;
	if (!bValidDuration)
		return false;

	SetFieldStep1_TargetQuery(InTargetQuery);
	SetFieldStep2_Source(InSourceUnit);
	SetFieldStep3_FollowingSource(InSourceUnit);
	SetFieldStep4_EffectSpec(InEffectSpecs);
	AutoFitVisualMeshToTargetQuery();
	SetActorHiddenInGame(FieldConfig.ApplicationPolicy == ESpGameplayFieldApplicationPolicy::Once);
	
	// init done. activate
	bFieldActive = true;
	
	K2_OnGameplayFieldActivated();
	ApplyVisualMeshRelativeZ();
	ForceNetUpdate();
	
	if (FieldConfig.ApplicationPolicy == ESpGameplayFieldApplicationPolicy::Once || FieldConfig.bEvaluateImmediately)
		EvaluateGameplayField();

	StartFieldTimers();
	return true;
}

void ASpGameplayField::SetFieldStep1_TargetQuery(const FSpTargetQuery& InTargetQuery)
{
	TargetQuery = InTargetQuery;

	FVector FieldDirection = TargetQuery.Shape != ESpTargetShape::Segment ? TargetQuery.Direction : TargetQuery.SegmentEnd - TargetQuery.Origin;
	FieldDirection.Z = 0.0f;
	
	const FRotator FieldRotation = FieldDirection.IsNearlyZero() ? FRotator::ZeroRotator : FieldDirection.Rotation();
	SetActorLocationAndRotation(TargetQuery.Origin, FieldRotation);
}

void ASpGameplayField::SetFieldStep2_Source(ASpUnit* InSourceUnit)
{
	SourceUnitUid = InSourceUnit->GetUnitUid();
	SourceTeamId = InSourceUnit->GetGenericTeamId();
	SourceUnavailableHandle = InSourceUnit->OnSourceUnavailable.AddUObject(this, &ThisClass::HandleSourceUnavailable);

	SetOwner(InSourceUnit);
	SetInstigator(Cast<APawn>(InSourceUnit));
	
	if (InSourceUnit->CheckDead() || InSourceUnit->GetUnitUid() != SourceUnitUid)
	{
		HandleSourceUnavailable();
		return;
	}
	
	SetFieldStep3_FollowingSource(InSourceUnit);
}

void ASpGameplayField::SetFieldStep3_FollowingSource(ASpUnit* InSourceUnit)
{
	bFollowingSource = FieldConfig.BindingPolicy == ESpGameplayFieldBindingPolicy::FollowSource;
	if (!bFollowingSource)
		return;

	const FTransform SourceTransform = InSourceUnit->GetActorTransform();
	LocalQueryOrigin = SourceTransform.InverseTransformPosition(TargetQuery.Origin);
	LocalQueryDirection = SourceTransform.InverseTransformVectorNoScale(TargetQuery.Direction);
	LocalQuerySegmentEnd = SourceTransform.InverseTransformPosition(TargetQuery.SegmentEnd);
		
	if (!AttachToActor(InSourceUnit, FAttachmentTransformRules::KeepWorldTransform))
		HandleSourceUnavailable();
	else if (!bFollowingSource)
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ASpGameplayField::SetFieldStep4_EffectSpec(const TArray<FGameplayEffectSpecHandle>& InEffectSpecs)
{
	EffectSpecs.Reserve(InEffectSpecs.Num());
	for (const FGameplayEffectSpecHandle& InputSpec : InEffectSpecs)
	{
		if (!InputSpec.IsValid())
			continue;

		FGameplayEffectSpec* FieldSpec = new FGameplayEffectSpec(*InputSpec.Data.Get());
		EffectSpecs.Add(FGameplayEffectSpecHandle(FieldSpec));
	}
}

void ASpGameplayField::StartFieldTimers()
{
	const bool bIsOnce = FieldConfig.ApplicationPolicy == ESpGameplayFieldApplicationPolicy::Once;
	if (!bIsOnce || bFollowingSource)
	{
		const float EvaluationPeriod = FMath::Max(FieldConfig.EvaluationPeriod, 0.01f);

		if (bIsOnce)
			GetWorldTimerManager().SetTimer(EvaluationTimerHandle, this, &ASpGameplayField::RefreshField, EvaluationPeriod, true, EvaluationPeriod);
		else
			GetWorldTimerManager().SetTimer(EvaluationTimerHandle, this, &ASpGameplayField::EvaluateGameplayField, EvaluationPeriod, true, EvaluationPeriod);
	}

	GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ASpGameplayField::FinishGameplayField, FieldConfig.Duration, false);
}

void ASpGameplayField::SetVisualMeshGroundOffset(const ASpUnit* InSourceUnit, const FVector& InOrigin)
{
	const UCapsuleComponent* Capsule = InSourceUnit->GetCapsuleComponent();
	if (!IsValid(Capsule))
		return;

	const float SourceGroundZ = InSourceUnit->GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight();
	const float WorldOffsetZ = SourceGroundZ - InOrigin.Z;

	TArray<UStaticMeshComponent*> VisualMeshes;
	GetComponents(VisualMeshes);
	if (VisualMeshes.Num() != 1)
		return;

	UStaticMeshComponent* VisualMesh = VisualMeshes[0];
	const USceneComponent* Parent = IsValid(VisualMesh) ? VisualMesh->GetAttachParent() : nullptr;
	if (!IsValid(VisualMesh) || VisualMesh->GetOwner() != this || !IsValid(Parent))
		return;

	const float ParentScaleZ = Parent->GetComponentScale().Z;
	if (FMath::IsNearlyZero(ParentScaleZ))
		return;

	FVector MeshLocation = VisualMesh->GetRelativeLocation();
	MeshLocation.Z += WorldOffsetZ / ParentScaleZ;
	VisualMesh->SetRelativeLocation(MeshLocation);
	VisualMeshRelativeZ = MeshLocation.Z;
}

void ASpGameplayField::ApplyVisualMeshRelativeZ()
{
	if (VisualMeshRelativeZ == TNumericLimits<float>::Max())
		return;

	TArray<UStaticMeshComponent*> VisualMeshes;
	GetComponents(VisualMeshes);
	if (VisualMeshes.Num() != 1)
		return;

	UStaticMeshComponent* VisualMesh = VisualMeshes[0];
	if (!IsValid(VisualMesh) || VisualMesh->GetOwner() != this)
		return;

	FVector MeshLocation = VisualMesh->GetRelativeLocation();
	MeshLocation.Z = VisualMeshRelativeZ;
	VisualMesh->SetRelativeLocation(MeshLocation);
}

// refresh

void ASpGameplayField::RefreshField()
{
	if (!CheckCanContinueField())
		return;

	RefreshBoundTargetQuery();
}

void ASpGameplayField::RefreshBoundTargetQuery()
{
	if (!bFollowingSource)
		return;

	const ASpUnit* Source = GetOwner<ASpUnit>();
	if (!Source)
		return;

	const FTransform SourceTransform = Source->GetActorTransform();
	TargetQuery.Origin = SourceTransform.TransformPosition(LocalQueryOrigin);
	TargetQuery.Direction = SourceTransform.TransformVectorNoScale(LocalQueryDirection);
	TargetQuery.SegmentEnd = SourceTransform.TransformPosition(LocalQuerySegmentEnd);
}

void ASpGameplayField::AutoFitVisualMeshToTargetQuery()
{
	if (!bAutoFitSingleVisualMesh)
		return;

	TArray<UStaticMeshComponent*> VisualMeshes;
	GetComponents(VisualMeshes);
	
	if (VisualMeshes.Num() != 1)
		return;

	UStaticMeshComponent* VisualMesh = VisualMeshes[0];
	bool bValidVisualMesh = IsValid(VisualMesh) && VisualMesh->GetOwner() == this;
	
	if (!bValidVisualMesh || !TargetQuery.IsValid())
		return;

	const UStaticMesh* StaticMesh = VisualMesh->GetStaticMesh();
	const bool bValidMeshOrientation = VisualMesh->GetRelativeRotation().IsNearlyZero();
	
	if (!StaticMesh || !bValidMeshOrientation)
		return;

	float VisualLength = 0.0f;
	float VisualWidth = 0.0f;
	float CenterOffsetX = 0.0f;
	
	if (!GetVisualQueryDimensions(VisualLength, VisualWidth, CenterOffsetX))
		return;

	const FBox MeshBounds = StaticMesh->GetBoundingBox();
	const FVector MeshSize = MeshBounds.GetSize();
	const USceneComponent* Parent = VisualMesh->GetAttachParent();
	const FVector ParentWorldScale = Parent ? Parent->GetComponentScale() : FVector::OneVector;
	
	const bool bValidVisualSize = VisualLength > 0.0f && VisualWidth > 0.0f;
	const bool bValidMeshSize = MeshSize.X > 0.0f && MeshSize.Y > 0.0f;
	const bool bValidParentScale = ParentWorldScale.X > 0.0f && ParentWorldScale.Y > 0.0f;
	
	if (!bValidVisualSize || !bValidMeshSize || !bValidParentScale)
		return;

	FVector MeshScale = VisualMesh->GetRelativeScale3D();
	MeshScale.X = VisualLength / (MeshSize.X * ParentWorldScale.X);
	MeshScale.Y = VisualWidth / (MeshSize.Y * ParentWorldScale.Y);
	VisualMesh->SetRelativeScale3D(MeshScale);

	const FVector MeshCenter = MeshBounds.GetCenter();
	FVector MeshLocation = VisualMesh->GetRelativeLocation();
	MeshLocation.X = CenterOffsetX / ParentWorldScale.X - MeshCenter.X * MeshScale.X;
	MeshLocation.Y = -MeshCenter.Y * MeshScale.Y;
	VisualMesh->SetRelativeLocation(MeshLocation);
}

// target

void ASpGameplayField::ProcessCollectedTargets(const TArray<ASpUnit*>& CollectedTargets)
{
	TSet<TWeakObjectPtr<ASpUnit>> NewTargets;
	NewTargets.Reserve(CollectedTargets.Num());

	for (ASpUnit* Target : CollectedTargets)
	{
		if (IsValid(Target))
			NewTargets.Add(TWeakObjectPtr<ASpUnit>(Target));
	}

	NotifyExitedTargets(NewTargets);

	for (ASpUnit* Target : CollectedTargets)
		ProcessCollectedTarget(Target);

	CurrentTargets = MoveTemp(NewTargets);
}

void ASpGameplayField::ProcessCollectedTarget(ASpUnit* Target)
{
	if (!IsValid(Target))
		return;

	const TWeakObjectPtr<ASpUnit> TargetKey(Target);
	const bool bNewTarget = !CurrentTargets.Contains(TargetKey);
	if (bNewTarget)
		HandleTargetEntered(Target);

	if (ShouldApplyEffectsToTarget(bNewTarget))
		ApplyFieldEffectsToTarget(Target);
}

void ASpGameplayField::NotifyExitedTargets(const TSet<TWeakObjectPtr<ASpUnit>>& RemainingTargets)
{
	for (const TWeakObjectPtr<ASpUnit>& PreviousTarget : CurrentTargets)
	{
		ASpUnit* Target = PreviousTarget.Get();
		if (IsValid(Target) && !RemainingTargets.Contains(PreviousTarget))
			HandleTargetExited(Target);
	}
}

void ASpGameplayField::HandleTargetEntered(ASpUnit* Target)
{
	K2_OnTargetEntered(Target);
}

void ASpGameplayField::HandleTargetExited(ASpUnit* Target)
{
	if (FieldConfig.bRemoveAppliedEffectsOnExit)
		RemoveTrackedEffectsFromTarget(Target);

	K2_OnTargetExited(Target);
}

void ASpGameplayField::HandleSourceUnavailable()
{
	for (FGameplayEffectSpecHandle& EffectSpec : EffectSpecs)
	{
		if (EffectSpec.IsValid())
			EffectSpec.Data->AddDynamicAssetTag(SpGameplayTags::EffectBehaviorTag_NoInstigator);
	}
	
	if (bFollowingSource)
	{
		RefreshBoundTargetQuery();
		bFollowingSource = false;
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	UnbindSourceUnavailable();
	SetOwner(nullptr);
	SetInstigator(nullptr);
	ForceNetUpdate();
}

// remove

void ASpGameplayField::ReleaseFailedGameplayField()
{
	if (IsActorBeingDestroyed())
		return;

	if (HasAuthority())
		DestroyGameplayField();
	else
		ReleaseGameplayField();
}

void ASpGameplayField::RemoveEffectsOnFinishIfConfigured()
{
	if (HasAuthority() && FieldConfig.bRemoveAppliedEffectsOnFinish)
		RemoveAllTrackedEffects();
}

void ASpGameplayField::RemoveTrackedEffectsFromTarget(ASpUnit* Target)
{
	if (!IsValid(Target))
		return;

	const TWeakObjectPtr<ASpUnit> TargetKey(Target);
	TArray<FActiveGameplayEffectHandle> Handles;

	if (!AppliedEffectHandles.RemoveAndCopyValue(TargetKey, Handles))
		return;

	RemoveEffectHandlesFromTarget(Target, Handles);
}

void ASpGameplayField::RemoveAllTrackedEffects()
{
	for (TPair<TWeakObjectPtr<ASpUnit>, TArray<FActiveGameplayEffectHandle>>& Pair : AppliedEffectHandles)
		RemoveEffectHandlesFromTarget(Pair.Key.Get(), Pair.Value);

	AppliedEffectHandles.Reset();
}

void ASpGameplayField::ClearFieldTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EvaluationTimerHandle);
		World->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	}
}

void ASpGameplayField::UnbindSourceUnavailable()
{
	if (!SourceUnavailableHandle.IsValid())
		return;

	if (ASpUnit* Source = GetOwner<ASpUnit>())
		Source->OnSourceUnavailable.Remove(SourceUnavailableHandle);
	
	SourceUnavailableHandle.Reset();
}

// helper

bool ASpGameplayField::CheckCanContinueField() const
{
	return HasAuthority() && bFieldActive && !IsActorBeingDestroyed();
}

bool ASpGameplayField::ShouldApplyEffectsToTarget(bool bNewTarget) const
{
	switch (FieldConfig.ApplicationPolicy)
	{
		case ESpGameplayFieldApplicationPolicy::Once:
		case ESpGameplayFieldApplicationPolicy::Periodic:
			return true;
		case ESpGameplayFieldApplicationPolicy::OnEnter:
			return bNewTarget;
		default:
			return false;
	}
}

bool ASpGameplayField::GetVisualQueryDimensions(float& OutLength, float& OutWidth, float& OutCenterOffsetX) const
{
	switch (TargetQuery.Shape)
	{
		case ESpTargetShape::Box:
		{
			OutLength = TargetQuery.Length;
			OutWidth = TargetQuery.Width;
			OutCenterOffsetX = TargetQuery.Length * 0.5f;
			break;
		}

		case ESpTargetShape::Circle:
		{
			OutLength = TargetQuery.Radius * 2.0f;
			OutWidth = OutLength;
			OutCenterOffsetX = 0.0f;
			break;
		}

		default:
			return false;
	}
	return true;
}

// rep

void ASpGameplayField::OnRep_TargetQuery()
{
	AutoFitVisualMeshToTargetQuery();
}

void ASpGameplayField::OnRep_FieldActive()
{
	AutoFitVisualMeshToTargetQuery();
	
	if (bFieldActive)
	{
		K2_OnGameplayFieldActivated();
		ApplyVisualMeshRelativeZ();
	}
	else
		K2_OnGameplayFieldFinished();
}

void ASpGameplayField::OnRep_VisualMeshRelativeZ()
{
	ApplyVisualMeshRelativeZ();
}
