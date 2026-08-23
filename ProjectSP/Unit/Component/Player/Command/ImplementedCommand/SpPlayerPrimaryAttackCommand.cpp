#include "SpPlayerPrimaryAttackCommand.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandRuntime.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandService_Movement.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandTypes.h"

struct FSpPlayerCommandContext_Server;

// ==================================================

bool FSpPlayerPrimaryAttackRules::IsTargetInRange(const ASpUnit& SourceUnit, const ASpUnit& TargetUnit) const
{
	return FVector::DistSquared2D(SourceUnit.GetActorLocation(), TargetUnit.GetActorLocation()) <= FMath::Square(AttackRange);
}

bool FSpPlayerPrimaryAttackRules::NeedsChasePath(const FSpPlayerCommandService_Movement& MovementService, const FVector& TargetLocation) const
{
	return !MovementService.HasPath_Server()
		|| FVector::DistSquared2D(TargetLocation, MovementService.GetPathEnd_Server()) > FMath::Square(ChaseRepathDistance);
}

// ==================================================

FSpPlayerPrimaryAttackCommand::FSpPlayerPrimaryAttackCommand(const uint16 InCommandId, ASpUnit* InTargetUnit, FSpPlayerPrimaryAttackRules InRules)
	: CommandId(InCommandId)
	, TargetUnit(InTargetUnit)
	, Rules(MoveTemp(InRules))
{
}

AActor* FSpPlayerPrimaryAttackCommand::GetTargetActor() const
{
	return TargetUnit.Get();
}

bool FSpPlayerPrimaryAttackCommand::Begin(FSpPlayerCommandContext_Server& Context)
{
	return TargetUnit.IsValid();
}

bool FSpPlayerPrimaryAttackCommand::Update(FSpPlayerCommandContext_Server& Context, const FSpPlayerCommandResolvedInput& Input)
{
	return true;
}

bool FSpPlayerPrimaryAttackCommand::Tick(FSpPlayerCommandContext_Server& Context, const float DeltaTime)
{
	ASpUnit* Unit = &Context.Unit;
	ASpUnit* Target = TargetUnit.Get();
	if (!Unit || !Target || !Target->IsAttackable(Unit))
		return false;

	FSpPlayerCommandService_Movement& MovementService = Context.MovementService;
	const FVector TargetLocation = Target->GetActorLocation();
	if (!Rules.IsTargetInRange(*Unit, *Target))
	{
		if (Rules.NeedsChasePath(MovementService, TargetLocation) && !MovementService.RebuildPath_Server(*Unit, CommandId, TargetLocation))
			return false;

		if (!MovementService.Tick_Server(*Unit, CommandId, DeltaTime))
			MovementService.StopFollowing_Server(*Unit, CommandId);

		return true;
	}

	if (MovementService.HasPath_Server())
		MovementService.StopFollowing_Server(*Unit, CommandId);

	if (UCharacterMovementComponent* MovementComponent = Unit->GetCharacterMovement())
		MovementComponent->StopMovementImmediately();

	Unit->RotateToTargetLocation(TargetLocation, DeltaTime, 1.0f);
	return true;
}

ESpPlayerCommandAbilityValidation FSpPlayerPrimaryAttackCommand::ValidateAbility(FSpPlayerCommandContext_Server& Context, const FGameplayTag AbilityTag)
{
	if (!AbilityTag.MatchesTagExact(FSpGameplayTags::Get().AbilitySlotTag_BasicAttack))
		return ESpPlayerCommandAbilityValidation::Rejected;

	ASpUnit* Unit = &Context.Unit;
	ASpUnit* Target = TargetUnit.Get();
	
	if (!Unit || !Target || !Target->IsAttackable(Unit))
		return ESpPlayerCommandAbilityValidation::EndCommand;

	return Rules.IsTargetInRange(*Unit, *Target) ? ESpPlayerCommandAbilityValidation::Accepted : ESpPlayerCommandAbilityValidation::Rejected;
}
