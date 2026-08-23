#include "SpPlayerCommandService_Movement.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

// server

bool FSpPlayerCommandService_Movement::Tick_Server(ASpUnit& Unit, const uint16 CommandId, const float /*DeltaTime*/)
{
	if (!HasPath_Server())
		return false;

	FVector Destination = ServerPathPoints[ServerPathIndex];
	FVector ToTarget = Destination - Unit.GetActorLocation();
	ToTarget.Z = 0.0f;
	
	if (ToTarget.SizeSquared() <= FMath::Square(Settings.EndDistance))
	{
		++ServerPathIndex;
		if (!HasPath_Server())
			return false;

		Destination = ServerPathPoints[ServerPathIndex];
		ToTarget = Destination - Unit.GetActorLocation();
		ToTarget.Z = 0.0f;
		SendMoveStateToClient_Server(Unit, CommandId, true, Destination);
	}

	return true;
}

bool FSpPlayerCommandService_Movement::UpdatePathDestination_Server(ASpUnit& Unit, const uint16 CommandId, const FVector& Destination)
{
	return !ShouldRepath_Server(Unit, Destination) || RebuildPath_Server(Unit, CommandId, Destination);
}

bool FSpPlayerCommandService_Movement::RebuildPath_Server(ASpUnit& Unit, const uint16 CommandId, const FVector& Destination)
{
	if (!Unit.HasAuthority())
		return false;

	ClearPath_Server();
	if (const UWorld* World = Unit.GetWorld())
		NextServerRepathTime = World->GetTimeSeconds() + Settings.RepathInterval;

	if (FVector::DistSquared2D(Unit.GetActorLocation(), Destination) <= FMath::Square(Settings.EndDistance))
		return true;

	UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(Unit.GetWorld(), Unit.GetActorLocation(), Destination, &Unit);
	if (!Path || Path->PathPoints.Num() < 2)
		return false;

	ServerPathPoints = Path->PathPoints;
	ServerPathIndex = 1;
	SendMoveStateToClient_Server(Unit, CommandId, true, ServerPathPoints[ServerPathIndex]);
	return true;
}

void FSpPlayerCommandService_Movement::StopFollowing_Server(ASpUnit& Unit, const uint16 CommandId)
{
	ClearPath_Server();
	SendMoveStateToClient_Server(Unit, CommandId, false, FVector::ZeroVector);
}

void FSpPlayerCommandService_Movement::StopCommand_Server(ASpUnit& Unit, const uint16 CommandId)
{
	StopFollowing_Server(Unit, CommandId);

	if (UCharacterMovementComponent* MovementComponent = Unit.GetCharacterMovement())
		MovementComponent->StopMovementImmediately();
}

// client

void FSpPlayerCommandService_Movement::BeginCommand_Client(const uint16 CommandId)
{
	ClientCommandId = CommandId;
	bClientHasWaypoint = false;
	ClientWaypoint = FVector::ZeroVector;
}

void FSpPlayerCommandService_Movement::ApplyMoveState_Client(const uint16 CommandId, const bool bHasWaypoint, const FVector& Waypoint)
{
	if (CommandId == 0 || CommandId != ClientCommandId)
		return;

	bClientHasWaypoint = bHasWaypoint;
	ClientWaypoint = bHasWaypoint ? Waypoint : FVector::ZeroVector;
}

void FSpPlayerCommandService_Movement::EndCommand_Client(const uint16 CommandId)
{
	if (CommandId == ClientCommandId)
		ClearClientState_Client();
}

void FSpPlayerCommandService_Movement::ClearClientState_Client()
{
	ClientCommandId = 0;
	bClientHasWaypoint = false;
	ClientWaypoint = FVector::ZeroVector;
}

bool FSpPlayerCommandService_Movement::Tick_Client(ASpUnit& Unit, const float /*DeltaTime*/) const
{
	if (!bClientHasWaypoint)
		return true;

	ASpPlayerController* PlayerController = Unit.GetController<ASpPlayerController>();
	if (!Unit.IsLocallyControlled() || !PlayerController || !PlayerController->IsGameplayInputEnabled_Client())
		return false;

	FVector ToWaypoint = ClientWaypoint - Unit.GetActorLocation();
	ToWaypoint.Z = 0.0f;
	if (ToWaypoint.SizeSquared() <= FMath::Square(Settings.EndDistance))
		return true;

	Unit.AddMovementInput(ToWaypoint.GetSafeNormal2D(), 1.0f);
	return true;
}

// check

bool FSpPlayerCommandService_Movement::ShouldRepath_Server(const ASpUnit& Unit, const FVector& Destination) const
{
	if (!HasPath_Server())
		return true;

	if (FVector::DistSquared2D(Destination, ServerPathPoints.Last()) < FMath::Square(Settings.RepathDistance))
		return false;

	const UWorld* World = Unit.GetWorld();
	return !World || World->GetTimeSeconds() >= NextServerRepathTime;
}

bool FSpPlayerCommandService_Movement::HasPath_Server() const
{
	return ServerPathPoints.IsValidIndex(ServerPathIndex);
}

const FVector& FSpPlayerCommandService_Movement::GetPathEnd_Server() const
{
	check(HasPath_Server());
	return ServerPathPoints.Last();
}

// ------------------------------------------------

void FSpPlayerCommandService_Movement::SendMoveStateToClient_Server(ASpUnit& Unit, const uint16 CommandId, const bool bHasWaypoint, const FVector& Waypoint) const
{
	if (CommandId == 0)
		return;

	if (ASpPlayerController* PlayerController = Unit.GetController<ASpPlayerController>())
		PlayerController->ClientSetCommandMoveState(CommandId, bHasWaypoint, Waypoint);
}

void FSpPlayerCommandService_Movement::ClearPath_Server()
{
	ServerPathPoints.Reset();
	ServerPathIndex = 0;
	NextServerRepathTime = 0.0f;
}
