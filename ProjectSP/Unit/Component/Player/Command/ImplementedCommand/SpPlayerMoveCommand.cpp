#include "SpPlayerMoveCommand.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandRuntime.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandService_Movement.h"

// ==================================================

FSpPlayerMoveCommand::FSpPlayerMoveCommand(const uint16 InCommandId, const FVector& InMoveDestination)
	: CommandId(InCommandId), MoveDestination(InMoveDestination)
{
}

bool FSpPlayerMoveCommand::Begin(FSpPlayerCommandContext_Server& Context)
{
	return Context.MovementService.RebuildPath_Server(Context.Unit, CommandId, MoveDestination);
}

bool FSpPlayerMoveCommand::Update(FSpPlayerCommandContext_Server& Context, const FSpPlayerCommandResolvedInput& Input)
{
	if (Input.Sequence == 0 || static_cast<int16>(Input.Sequence - LastCursorSequence) <= 0)
		return true;

	if (!Input.bHasMoveDestination)
		return true;

	LastCursorSequence = Input.Sequence;
	MoveDestination = Input.MoveDestination;
	return Context.MovementService.UpdatePathDestination_Server(Context.Unit, CommandId, MoveDestination);
}

bool FSpPlayerMoveCommand::Tick(FSpPlayerCommandContext_Server& Context, const float DeltaTime)
{
	return Context.MovementService.Tick_Server(Context.Unit, CommandId, DeltaTime);
}

ESpPlayerCommandAbilityValidation FSpPlayerMoveCommand::ValidateAbility(FSpPlayerCommandContext_Server& Context, const FGameplayTag AbilityTag)
{
	return ESpPlayerCommandAbilityValidation::Rejected;
}
