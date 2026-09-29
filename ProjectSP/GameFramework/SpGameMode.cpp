#include "SpGameMode.h"
#include "SpPlayerController.h"
#include "SpPlayerState.h"
#include "Engine/World.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/GameFramework/GameState/SpGamePhaseState.h"
#include "ProjectSP/GameFramework/GameState/SpGamePhaseLobbyState.h"
#include "ProjectSP/Subsystem/SpawnSubsystem.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/Define/SpTeam.h"

// ==================================================

ASpGameMode::ASpGameMode() : PhaseMachine(Phase)
{
	GameStateClass = ASpGameState::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ASpGameMode::InitGameState()
{
	Super::InitGameState();

	if (ensure(GetGameState<ASpGameState>()))
		PhaseMachine.Begin<FSpGamePhaseLobbyState>(*this);
}

void ASpGameMode::StartPlay()
{
	Super::StartPlay();

	if (FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>())
		State->StartPlay();
}

void ASpGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PhaseMachine.End();
	Super::EndPlay(EndPlayReason);
}

void ASpGameMode::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>())
	{
		// 상태의 처리가 반환된 뒤 교체한다. 콜백 실행 중 상태 객체를 파괴하지 않는다.
		if (TUniquePtr<ISpState<ESpGamePhase>> NextState = State->Update())
			PhaseMachine.TryChangeState(MoveTemp(NextState));
	}
}

void ASpGameMode::Logout(AController* Exiting)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(Exiting);
	
	if (State && PlayerController)
		State->BeforePlayerLogout(*PlayerController);

	Super::Logout(Exiting);

	State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->AfterPlayerLogout();
}

// override gameMode

bool ASpGameMode::ReadyToStartMatch_Implementation()
{
	return PhaseMachine.IsActive() && Phase == ESpGamePhase::Playing;
}

void ASpGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(NewPlayer);

	if (!State || !PlayerController)
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}

	const bool bRunDefaultStart = State->BeforePlayerJoin(*PlayerController);

	if (bRunDefaultStart)
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	State->HandlePlayerJoined(*PlayerController);
}

UClass* ASpGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ASpPlayerState* PlayerState = InController->GetPlayerState<ASpPlayerState>())
	{
		if (USpUnitDefinition* UnitDefinition = PlayerState->GetUnitDefinition())
			return UnitDefinition->UnitClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ASpGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	const ASpPlayerController* PlayerController = Cast<ASpPlayerController>(NewPlayer);
	const ASpPlayerState* PlayerState = NewPlayer->GetPlayerState<ASpPlayerState>();

	if (!PlayerController || !PlayerState)
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

	if (USpUnitDefinition* UnitDefinition = PlayerState->GetUnitDefinition())
	{
		if (USpawnSubsystem* SpawnSubsystem = GetWorld()->GetSubsystem<USpawnSubsystem>())
		{
			bool bActivateImmediately = Phase == ESpGamePhase::Playing;
			FGenericTeamId TeamID = FGenericTeamId(SpTeam::PlayerId);

			return SpawnSubsystem->SpawnPlayerUnit(UnitDefinition, SpawnTransform, SpawnInfo, TeamID, bActivateImmediately);
		}
		return nullptr;
	}

	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

// handle

void ASpGameMode::HandlePlayerReadyChanged_Server(ASpPlayerController* PlayerController, const bool bReadyToStart)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->HandlePlayerReadyChanged(*PlayerController, bReadyToStart);
}

void ASpGameMode::HandleRoomStartRequest_Server(ASpPlayerController* PlayerController)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->HandleStartRequested(*PlayerController);
}

void ASpGameMode::HandleInitialPresentationReady_Server(ASpPlayerController* PlayerController, const uint32 InInitialPresentationId)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->HandlePresentationReady(*PlayerController, InInitialPresentationId);
}

bool ASpGameMode::HandlePlayableUnitSelection_Server(ASpPlayerController* PlayerController, USpUnitDefinition* UnitDefinition)
{
	if (!HasAuthority() || !IsValid(PlayerController) || PlayerController->GetWorld() != GetWorld())
		return false;

	ASpPlayerState* PlayerState = PlayerController->GetPlayerState<ASpPlayerState>();
	const ASpGameState* SpGameState = GetGameState<ASpGameState>();

	if (!PlayerState || !SpGameState || !SpGameState->IsPlayableUnitDefinition(UnitDefinition))
		return false;

	if (Phase == ESpGamePhase::Lobby)
	{
		PlayerState->SetSelectedUnitDefinition_Server(UnitDefinition);
		return true;
	}

	if (Phase != ESpGamePhase::Playing)
		return false;
	
	return ReplacePlayableUnit_Server(*PlayerController, *PlayerState, UnitDefinition);
}

bool ASpGameMode::ReplacePlayableUnit_Server(ASpPlayerController& PlayerController, ASpPlayerState& PlayerState, USpUnitDefinition* UnitDefinition)
{
	ASpPlayerUnit* PreviousUnit = PlayerController.GetPawn<ASpPlayerUnit>();
	if (!IsValid(PreviousUnit) || PreviousUnit->GetController() != &PlayerController || !PreviousUnit->HasValidUnitData())
		return false;

	if (PreviousUnit->GetUnitDefinition() == UnitDefinition)
	{
		PlayerState.SetSelectedUnitDefinition_Server(UnitDefinition);
		return true;
	}

	USpawnSubsystem* SpawnSubsystem = GetWorld()->GetSubsystem<USpawnSubsystem>();
	if (!SpawnSubsystem)
		return false;

	ASpPlayerUnit* NewUnit = SpawnReplacementUnit_Server(*SpawnSubsystem, *PreviousUnit, UnitDefinition);
	if (!NewUnit)
		return false;

	PlayerController.Possess(NewUnit);
	if (PlayerController.GetPawn() != NewUnit)
	{
		NewUnit->Destroy();
		if (IsValid(PreviousUnit) && PlayerController.GetPawn() != PreviousUnit)
			PlayerController.Possess(PreviousUnit);

		return false;
	}

	SpawnSubsystem->ActivateUnit(NewUnit, false);
	PlayerState.SetSelectedUnitDefinition_Server(UnitDefinition);
	PreviousUnit->Destroy();

	return true;
}

ASpPlayerUnit* ASpGameMode::SpawnReplacementUnit_Server(USpawnSubsystem& SpawnSubsystem, const ASpPlayerUnit& PreviousUnit, USpUnitDefinition* UnitDefinition)
{
	FTransform SpawnTransform = PreviousUnit.GetActorTransform();
	SpawnTransform.SetLocation(PreviousUnit.GetSpawnLocation());

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return SpawnSubsystem.SpawnPlayerUnit(UnitDefinition, SpawnTransform, SpawnInfo, FGenericTeamId(SpTeam::PlayerId), false);
}
