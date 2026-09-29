#include "SpPrimaryInputBehavior.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputDefinition.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/Component/Player/SpPlayerActionComponent.h"
#include "ProjectSP/Unit/Define/SpUnitGlobalValue.h"

// ==================================================

ESpInputBehaviorRequest USpPrimaryInputBehavior::HandleInput(ASpPlayerUnit& Unit, const FInputActionValue&, const ETriggerEvent TriggerEvent, const FSpInputActionBinding& Binding, const bool bIsActiveBehavior)
{
	if (TriggerEvent != ETriggerEvent::Started && !bIsActiveBehavior)
		return ESpInputBehaviorRequest::None;

	switch (TriggerEvent)
	{
		case ETriggerEvent::Started:
			return HandleInputBegin(Unit, Binding);

		case ETriggerEvent::Triggered:
			return HandleInputTriggered(Unit, Binding);

		case ETriggerEvent::Canceled:
		case ETriggerEvent::Completed:
			return HandleInputEnd(Unit);
		
		default:
			return ESpInputBehaviorRequest::None;
	}
}

void USpPrimaryInputBehavior::TickBehavior(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, const FGameplayTag SourceInputTag, const float DeltaTime)
{
	const uint64 CurrentRevision = StateRevision;
	
	if (ProcessPendingAbility(Unit, ActionComponent, SourceInputTag, DeltaTime) || CurrentRevision != StateRevision)
		return;

	if (ASpUnit* Target = Unit.GetTargetActor())
	{
		if (!Unit.IsAttackable(Target))
			return;

		const float AttackerRadius = Unit.GetCapsuleComponent()->GetScaledCapsuleRadius();
		const float TargetRadius = Target->GetCapsuleComponent()->GetScaledCapsuleRadius();
		const float StopDistance = SpUnitGlobalValue::FollowAcceptanceRadius + AttackerRadius + TargetRadius;

		Unit.MoveToTargetLocation(Target->GetActorLocation(), StopDistance, 1.0f);
		return;
	}

	if (Destination.IsSet())
	{
		Unit.MoveToTargetLocation(Destination.GetValue(), SpUnitGlobalValue::FollowAcceptanceRadius, 1.0f);
	}
}

void USpPrimaryInputBehavior::ClearBehavior(ASpPlayerUnit* Unit)
{
	if (Unit)
	{
		if (UCharacterMovementComponent* Movement = Unit->GetCharacterMovement())
			Movement->StopMovementImmediately();
	}

	Destination.Reset();
	AbilityTag = FGameplayTag();
	bAbilityActivationPending = false;
	
	++StateRevision;
}

// process

bool USpPrimaryInputBehavior::ProcessPendingAbility(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, const FGameplayTag SourceInputTag, const float DeltaTime)
{
	if (!AbilityTag.IsValid())
		return false;
	
	const USpAbilitySystemComponent* ASC = Unit.GetSpAbilitySystemComponent();
	if (ASC && ASC->IsAbilityTagActive(AbilityTag))
		return true;

	if (!bAbilityActivationPending || !AbilityTag.IsValid() || !ASC)
		return false;

	const bool bNeedTarget = ASC->HasAbilityAssetTag(AbilityTag, SpGameplayTags::AbilityTargetTag_Hostile);

	const uint64 CurrentRevision = StateRevision;
	if (bNeedTarget)
	{
		ASpUnit* Target = Unit.GetTargetActor();
		if (!Unit.IsAttackable(Target))
			return true;

		float ExecuteRange = 0.0f;
		if (!ASC->TryGetAbilityExecuteRangeByTag(AbilityTag, ExecuteRange))
			return true;

		if (ProcessTargetDistance(Unit, *Target, ExecuteRange))
			return true;

		if (ASC->HasAbilityAssetTag(AbilityTag, SpGameplayTags::AbilityConditionTag_TargetFacing) && ProcessTargetFacing(Unit, *Target, DeltaTime))
			return true;
	}

	if (CurrentRevision != StateRevision)
		return true;

	if (TryActivatePendingAbility(Unit, ActionComponent, SourceInputTag))
		return true;

	return bNeedTarget;
}

bool USpPrimaryInputBehavior::TryActivatePendingAbility(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, const FGameplayTag SourceInputTag)
{
	if (!bAbilityActivationPending || !AbilityTag.IsValid())
		return false;

	const uint64 CurrentRevision = StateRevision;
	const FGameplayTag CurrentAbilityTag = AbilityTag;
	
	USpAbilitySystemComponent* ASC = Unit.GetSpAbilitySystemComponent();
	if (!ASC || ASC->IsAbilityTagActive(CurrentAbilityTag) || !ASC->CanActivateAbilityByTag(CurrentAbilityTag))
		return false;

	if (CurrentRevision != StateRevision)
		return false;

	bool bNeedTarget = ASC->HasAbilityAssetTag(CurrentAbilityTag, SpGameplayTags::AbilityTargetTag_Hostile);
	bool bNeedFaceTarget = bNeedTarget && ASC->HasAbilityAssetTag(CurrentAbilityTag, SpGameplayTags::AbilityConditionTag_TargetFacing);
	ActionComponent.SyncTargetToServer(bNeedFaceTarget);
	
	if (CurrentRevision != StateRevision)
		return false;

	if (!ASC->TryActivateAbilityByTag(CurrentAbilityTag))
		return false;

	if (CurrentRevision != StateRevision)
		return true;

	if (!ActionComponent.IsInputHeld(SourceInputTag))
		bAbilityActivationPending = false;

	if (UCharacterMovementComponent* Movement = Unit.GetCharacterMovement())
		Movement->StopMovementImmediately();

	return true;
}

bool USpPrimaryInputBehavior::ProcessTargetDistance(ASpPlayerUnit& Unit, const ASpUnit& Target, const float ExecuteRange)
{
	const FVector TargetLocation = Target.GetActorLocation();
	if (FVector::DistSquared2D(Unit.GetActorLocation(), TargetLocation) <= FMath::Square(ExecuteRange))
	{
		if (UCharacterMovementComponent* Movement = Unit.GetCharacterMovement())
			Movement->StopMovementImmediately();
		return false;
	}

	Unit.MoveToTargetLocation(TargetLocation, ExecuteRange, 1.0f);
	return true;
}

bool USpPrimaryInputBehavior::ProcessTargetFacing(ASpPlayerUnit& Unit, const ASpUnit& Target, const float DeltaTime)
{
	const FVector TargetLocation = Target.GetActorLocation();
	if (Unit.IsFacingTargetLocation(TargetLocation))
		return false;

	if (UCharacterMovementComponent* Movement = Unit.GetCharacterMovement())
		Movement->StopMovementImmediately();

	Unit.RotateToTargetLocation(TargetLocation, DeltaTime, 1.0f);
	return !Unit.IsFacingTargetLocation(TargetLocation);
}

// handle

ESpInputBehaviorRequest USpPrimaryInputBehavior::HandleInputBegin(ASpPlayerUnit& Unit, const FSpInputActionBinding& Binding)
{
	APlayerController* PC = Cast<APlayerController>(Unit.GetController());
	FHitResult Hit;
	const bool bHasHit = PC && PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	ASpUnit* HitUnit = bHasHit ? Cast<ASpUnit>(Hit.GetActor()) : nullptr;
	const bool bAttackableHit = Unit.IsAttackable(HitUnit);

	Destination.Reset();
	Unit.SetTargetActor(bAttackableHit ? HitUnit : nullptr);
	AbilityTag = bAttackableHit ? Binding.AbilityTag : FGameplayTag();
	bAbilityActivationPending = AbilityTag.IsValid();
	
	++StateRevision;

	if (!bAttackableHit && bHasHit)
		Destination = Hit.ImpactPoint;

	return ESpInputBehaviorRequest::Set;
}

ESpInputBehaviorRequest USpPrimaryInputBehavior::HandleInputTriggered(ASpPlayerUnit& Unit, const FSpInputActionBinding& Binding)
{
	if (ASpUnit* Target = Unit.GetTargetActor())
	{
		if (!Unit.IsAttackable(Target))
			return HandleInputBegin(Unit, Binding);

		return ESpInputBehaviorRequest::None;
	}

	if (Destination.IsSet())
	{
		APlayerController* PC = Cast<APlayerController>(Unit.GetController());
		FHitResult Hit;

		if (PC && PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit))
			Destination = Hit.ImpactPoint;

		return ESpInputBehaviorRequest::None;
	}

	return HandleInputBegin(Unit, Binding);
}

ESpInputBehaviorRequest USpPrimaryInputBehavior::HandleInputEnd(ASpPlayerUnit& Unit)
{
	if (Unit.GetTargetActor() || Destination.IsSet())
		return ESpInputBehaviorRequest::None;
	
	return ESpInputBehaviorRequest::Clear;
}
