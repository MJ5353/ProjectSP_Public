#include "SpGamePhaseLobbyState.h"
#include "SpGamePhaseStartingState.h"
#include "Engine/World.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/GameFramework/SpPlayerState.h"
#include "ProjectSP/Subsystem/SpawnSubsystem.h"
#include "ProjectSP/Unit/SpAIUnit.h"

// ==================================================

FSpGamePhaseLobbyState::FSpGamePhaseLobbyState(ASpGameMode& InGameMode, const bool bInResetRoom)
	: FSpGamePhaseState(InGameMode), bResetRoom(bInResetRoom)
{
}

// core

void FSpGamePhaseLobbyState::Enter()
{
	GameState.InitialPresentationId = 0;
	GameState.InitialPresentationUnitUids.Reset();
	GameState.OnRep_InitialPresentation();

	FSpGamePhaseState::Enter();

	if (bResetRoom)
		ResetEmptyRoom();

	// 최초 준비는 StartPlay에서 수행하고, ResetRoom 이후 로비 재진입 시에는 접속자가 있을 때만 맵 유닛을 다시 준비한다.
	if (bResetRoom && !GameMode.ConnectedPlayers.IsEmpty())
		PrepareMapUnits();
	
	RefreshRoomHost();
	RefreshStartAvailability();
}

void FSpGamePhaseLobbyState::StartPlay()
{
	PrepareMapUnits();
	RefreshStartAvailability();
}

TUniquePtr<ISpState<ESpGamePhase>> FSpGamePhaseLobbyState::Update()
{
	ASpPlayerController* RequestingPlayer = StartRequestPlayer.Get();
	StartRequestPlayer.Reset();

	if (RequestingPlayer && RequestingPlayer == GameMode.RoomHostPlayer.Get() && CanStart())
		return MakeUnique<FSpGamePhaseStartingState>(GameMode);
	
	return nullptr;
}

// logout

void FSpGamePhaseLobbyState::AfterPlayerLogout()
{
	FSpGamePhaseState::AfterPlayerLogout();
	
	if (bRoomResetPending) // 방 초기화 요청이 남아있으면
	{
		ResetEmptyRoom();
		bRoomResetPending = false;
	}

	RefreshStartAvailability();
}

// joined

bool FSpGamePhaseLobbyState::BeforePlayerJoin(ASpPlayerController& PlayerController)
{
	PrepareMapUnits();
	return true;
}

// handle

void FSpGamePhaseLobbyState::HandlePlayerJoined(ASpPlayerController& PlayerController)
{
	GameMode.ConnectedPlayers.AddUnique(&PlayerController);
	GameMode.InitialPlayers.Add(&PlayerController);

	// 준비 상태를 미완료로 설정
	if (ASpPlayerState* PlayerState = PlayerController.GetPlayerState<ASpPlayerState>())
		PlayerState->SetReadyToStart_Server(false);

	if (ASpUnit* Unit = PlayerController.GetPawn<ASpUnit>())
		GameMode.InitialPlayerUnits.AddUnique(Unit);

	RefreshRoomHost();
	RefreshStartAvailability();
}

void FSpGamePhaseLobbyState::HandlePlayerReadyChanged(ASpPlayerController& PlayerController, const bool bReadyToStart)
{
	if (!GameMode.InitialPlayers.Contains(&PlayerController))
		return;

	if (ASpPlayerState* PlayerState = PlayerController.GetPlayerState<ASpPlayerState>())
		PlayerState->SetReadyToStart_Server(bReadyToStart);

	RefreshStartAvailability();
}

void FSpGamePhaseLobbyState::HandleStartRequested(ASpPlayerController& PlayerController)
{
	if (&PlayerController == GameMode.RoomHostPlayer.Get() && CanStart())
		StartRequestPlayer = &PlayerController;
}

void FSpGamePhaseLobbyState::PrepareMapUnits()
{
	if (GameMode.bMapUnitsPrepared)
		return;

	if (!ensureMsgf(GameMode.MapDefinition, TEXT("Map Definition이 링크되지 않음")))
		return;

	USpawnSubsystem* SpawnSubsystem = USpawnSubsystem::Get(GameMode.GetWorld());
	if (!SpawnSubsystem)
		return;

	TArray<ASpUnit*> SpawnedMapUnits;
	SpawnSubsystem->SpawnMapUnits(GameMode.MapDefinition, &SpawnedMapUnits, false);

	GameState.LobbyMapPresentationUnitUids.Reset();

	for (ASpUnit* Unit : SpawnedMapUnits)
	{
		if (!Unit)
			continue;

		GameMode.MapUnits.AddUnique(Unit);

		if (Unit->HasValidUnitData())
			GameState.LobbyMapPresentationUnitUids.AddUnique(Unit->GetUnitUid());
	}

	// 일괄 활성화
	for (ASpUnit* Unit : SpawnedMapUnits)
		SpawnSubsystem->ActivateUnit(Unit, true);
	
	GameMode.bMapUnitsPrepared = true;
	
	GameState.bLobbyMapUnitsActivated = true;
	GameState.OnRep_LobbyMapPresentation();
	GameState.ForceNetUpdate();
}

void FSpGamePhaseLobbyState::ResetEmptyRoom()
{
	for (const TWeakObjectPtr<ASpUnit>& Unit : GameMode.MapUnits)
	{
		if (ASpAIUnit* AIUnit = Cast<ASpAIUnit>(Unit.Get()))
			AIUnit->Push();
	}

	GameMode.MapUnits.Reset();
	GameMode.InitialPlayerUnits.Reset();
	GameMode.InitialPlayers.Reset();
	GameMode.bMapUnitsPrepared = false;
	
	StartRequestPlayer.Reset();

	GameState.LobbyMapPresentationUnitUids.Reset();
	GameState.bLobbyMapUnitsActivated = false;
	GameState.OnRep_LobbyMapPresentation();
	GameState.ForceNetUpdate();

	if (GameMode.GetMatchState() == MatchState::InProgress)
		GameMode.EndMatch();
	
	if (GameMode.GetMatchState() != MatchState::WaitingToStart)
		GameMode.SetMatchState(MatchState::WaitingToStart);
}

bool FSpGamePhaseLobbyState::CanStart() const
{
	if (!GameMode.bMapUnitsPrepared || !GameMode.RoomHostPlayer.IsValid())
		return false;

	bool bHasPlayer = false;
	
	for (const TWeakObjectPtr<ASpPlayerController>& Player : GameMode.InitialPlayers)
	{
		if (!Player.IsValid())
			continue;

		bHasPlayer = true;
		
		const ASpPlayerState* PlayerState = Player->GetPlayerState<ASpPlayerState>();
		if (!PlayerState || !PlayerState->IsReadyToStart())
			return false;
	}

	return bHasPlayer;
}

void FSpGamePhaseLobbyState::RefreshStartAvailability()
{
	const bool bCanStart = CanStart();
	
	if (GameState.bRoomStartAvailable != bCanStart)
	{
		GameState.bRoomStartAvailable = bCanStart;
		GameState.OnRep_RoomStartAvailable();
		GameState.ForceNetUpdate();
	}
}
