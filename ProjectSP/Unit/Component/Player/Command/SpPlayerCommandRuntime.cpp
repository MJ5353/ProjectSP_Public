#include "SpPlayerCommandRuntime.h"
#include "SpPlayerCommandService_Movement.h"
#include "SpPlayerCommandResolver.h"
#include "ImplementedCommand/SpPlayerMoveCommand.h"
#include "ImplementedCommand/SpPlayerPrimaryAttackPrediction.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

void FSpPlayerCommandRuntime_Server::HandleInput_Server(ASpUnit& Unit, const FSpPlayerCommandInput& CommandInput, const FSpPlayerCommandResolver& Resolver)
{
	check(MovementService);

	switch (CommandInput.Phase)
	{
		case ESpPlayerCommandInputPhase::Begin:
		{
			FSpPlayerCommandResolution Resolution;
			if (Resolver.Resolve_Server(Unit, CommandInput.CursorRay, Resolution))
				StartCommand_Server(Unit, CreateServerCommand(CommandInput.CommandId, Resolution, PrimaryAttackRules));
			else
				StopCommand_Server(Unit);
			break;
		}

		case ESpPlayerCommandInputPhase::Update:
		{
			if (!ActiveCommand || ActiveCommand->GetId() != CommandInput.CommandId)
				break;

			FSpPlayerCommandResolvedInput ResolvedInput;
			ResolvedInput.CommandId = CommandInput.CommandId;
			ResolvedInput.Sequence = CommandInput.Sequence;
			if (!ActiveCommand->GetTargetActor())
				ResolvedInput.bHasMoveDestination = Resolver.ResolveMoveDestination_Server(Unit, CommandInput.CursorRay, ResolvedInput.MoveDestination);

			FSpPlayerCommandContext_Server Context{ Unit, *MovementService };
			if (!ActiveCommand->Update(Context, ResolvedInput))
				StopCommand_Server(Unit);
			break;
		}

		case ESpPlayerCommandInputPhase::End:
		{
			if (ActiveCommand && ActiveCommand->GetId() == CommandInput.CommandId && ActiveCommand->ShouldEndOnInputEnd())
				StopCommand_Server(Unit);
			break;
		}
	}
}

void FSpPlayerCommandRuntime_Server::Tick_Server(ASpUnit& Unit, const float DeltaTime)
{
	check(MovementService);

	if (!ActiveCommand)
		return;

	FSpPlayerCommandContext_Server Context{ Unit, *MovementService };
	if (!ActiveCommand->Tick(Context, DeltaTime))
		StopCommand_Server(Unit);
}

bool FSpPlayerCommandRuntime_Server::ValidatePredictedAbility_Server(ASpUnit& Unit, const uint16 CommandId, const FGameplayTag AbilityTag)
{
	check(MovementService);

	if (!ActiveCommand || ActiveCommand->GetId() != CommandId)
		return false;

	FSpPlayerCommandContext_Server Context{ Unit, *MovementService };
	switch (ActiveCommand->ValidateAbility(Context, AbilityTag))
	{
		case ESpPlayerCommandAbilityValidation::Accepted:
			return true;

		case ESpPlayerCommandAbilityValidation::EndCommand:
			StopCommand_Server(Unit);
			return false;

		case ESpPlayerCommandAbilityValidation::Rejected:
		default:
			return false;
	}
}

void FSpPlayerCommandRuntime_Server::Clear_Server(ASpUnit& Unit)
{
	check(MovementService);

	StopCommand_Server(Unit);
}

TUniquePtr<ISpPlayerCommand> FSpPlayerCommandRuntime_Server::CreateServerCommand(const uint16 CommandId, const FSpPlayerCommandResolution& Resolution, const FSpPlayerPrimaryAttackRules& Rules)
{
	switch (Resolution.Kind)
	{
		case ESpPlayerCommandKind::PrimaryAttack:
			if (ASpUnit* TargetUnit = Resolution.TargetUnit.Get())
				return MakeUnique<FSpPlayerPrimaryAttackCommand>(CommandId, TargetUnit, Rules);
			return nullptr;

		case ESpPlayerCommandKind::Move:
		default:
			return MakeUnique<FSpPlayerMoveCommand>(CommandId, Resolution.MoveDestination);
	}
}

void FSpPlayerCommandRuntime_Server::StartCommand_Server(ASpUnit& Unit, TUniquePtr<ISpPlayerCommand> InCommand)
{
	check(MovementService);

	StopCommand_Server(Unit);
	ActiveCommand = MoveTemp(InCommand);
	if (!ActiveCommand)
		return;

	FSpPlayerCommandContext_Server Context{ Unit, *MovementService };
	if (!ActiveCommand->Begin(Context))
		StopCommand_Server(Unit);
}

void FSpPlayerCommandRuntime_Server::StopCommand_Server(ASpUnit& Unit)
{
	check(MovementService);

	const uint16 StoppedCommandId = ActiveCommand ? ActiveCommand->GetId() : 0;
	ActiveCommand.Reset();
	MovementService->StopCommand_Server(Unit, StoppedCommandId);
}

// ==================================================

void FSpPlayerCommandRuntime_Client::HandleInput_Client(ASpUnit& Unit, USpAbilitySystemComponent& ASC, const FSpPlayerCommandInput& CommandInput, const FSpPlayerCommandResolver& Resolver)
{
	check(MovementService);

	switch (CommandInput.Phase)
	{
		case ESpPlayerCommandInputPhase::Begin:
		{
			Clear_Client();
			ActiveCommandId = CommandInput.CommandId;
			MovementService->BeginCommand_Client(ActiveCommandId);

			const FSpPlayerCommandResolution Resolution = Resolver.Resolve_Client(Unit, CommandInput.CursorRay);
			ActivePrediction = CreatePrediction_Client(ActiveCommandId, Unit, ASC, Resolution, PrimaryAttackRules);
			break;
		}

		case ESpPlayerCommandInputPhase::End:
		{
			if (ActivePrediction && ActivePrediction->GetId() == CommandInput.CommandId && ActivePrediction->ShouldEndOnInputEnd())
				Clear_Client();
			break;
		}

		case ESpPlayerCommandInputPhase::Update:
		default:
			break;
	}
}

void FSpPlayerCommandRuntime_Client::Tick_Client(ASpUnit& Unit, const float DeltaTime)
{
	check(MovementService);

	if (ActiveCommandId == 0)
		return;

	if (MovementService->HasWaypoint_Client() && !MovementService->Tick_Client(Unit, DeltaTime))
	{
		Clear_Client();
		return;
	}

	if (ActivePrediction && !ActivePrediction->Tick(DeltaTime))
		Clear_Client();
}

void FSpPlayerCommandRuntime_Client::Clear_Client()
{
	check(MovementService);

	if (ActivePrediction)
		ActivePrediction->Cancel();

	ActivePrediction.Reset();
	ActiveCommandId = 0;
	MovementService->ClearClientState_Client();
}

bool FSpPlayerCommandRuntime_Client::ShouldTick_Client() const
{
	return ActivePrediction.IsValid() || (MovementService && MovementService->HasWaypoint_Client());
}

TUniquePtr<ISpPlayerCommandPrediction> FSpPlayerCommandRuntime_Client::CreatePrediction_Client(const uint16 CommandId, ASpUnit& SourceUnit, USpAbilitySystemComponent& ASC, 
	const FSpPlayerCommandResolution& Resolution, const FSpPlayerPrimaryAttackRules& Rules)
{
	if (Resolution.Kind != ESpPlayerCommandKind::PrimaryAttack)
		return nullptr;

	ASpUnit* TargetUnit = Resolution.TargetUnit.Get();
	if (!TargetUnit)
		return nullptr;

	return MakeUnique<FSpPlayerPrimaryAttackPrediction>(CommandId, &SourceUnit, TargetUnit, &ASC, Rules);
}