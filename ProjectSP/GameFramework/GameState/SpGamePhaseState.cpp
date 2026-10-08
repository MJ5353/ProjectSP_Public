#include "SpGamePhaseState.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/GameFramework/SpPlayerState.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

FSpGamePhaseState::FSpGamePhaseState(ASpGameMode& InGameMode) : GameMode(InGameMode), GameState(*InGameMode.GetGameState<ASpGameState>())
{
}

void FSpGamePhaseState::RefreshRoomHost()
{
	GameMode.ConnectedPlayers.RemoveAll([](const TWeakObjectPtr<ASpPlayerController>& Player)
	{
		return !Player.IsValid();
	});

	if (!GameMode.RoomHostPlayer.IsValid() || !GameMode.ConnectedPlayers.Contains(GameMode.RoomHostPlayer))
	{
		GameMode.RoomHostPlayer = GameMode.ConnectedPlayers.IsEmpty() ? nullptr : GameMode.ConnectedPlayers[0].Get();
	}

	ASpPlayerState* HostState = GameMode.RoomHostPlayer.IsValid() ? GameMode.RoomHostPlayer->GetPlayerState<ASpPlayerState>() : nullptr;
	if (GameState.RoomHostPlayerState != HostState)
	{
		GameState.RoomHostPlayerState = HostState;
		GameState.OnRep_RoomHostPlayerState();
	}
}

void FSpGamePhaseState::Enter()
{
	check(GameMode.HasAuthority());

	GameState.GamePhase = GetState();
	GameState.OnRep_GamePhase();
	GameState.ForceNetUpdate();
}

// logout

void FSpGamePhaseState::BeforePlayerLogout(ASpPlayerController& PlayerController)
{
	// 엔진 Logout에서 Pawn/PlayerState가 정리되기 전에 참여 목록에서 제거한다.
	GameMode.InitialPlayerUnits.Remove(PlayerController.GetPawn<ASpUnit>());
	GameMode.InitialPlayers.Remove(&PlayerController);
	GameMode.ConnectedPlayers.RemoveAll([&PlayerController](const TWeakObjectPtr<ASpPlayerController>& Player)
	{
		return !Player.IsValid() || Player.Get() == &PlayerController;
	});
	
	bRoomResetPending |= GameMode.ConnectedPlayers.IsEmpty();
}

void FSpGamePhaseState::AfterPlayerLogout()
{
	RefreshRoomHost();
}

// joined

void FSpGamePhaseState::HandlePlayerJoined(ASpPlayerController& PlayerController)
{
	// 중도 접속 처리. preparing state 외의 접속자는 이번 게임의 준비 대상에 추가하지 않는다.

	GameMode.ConnectedPlayers.AddUnique(&PlayerController);
	PlayerController.StartSpectatingOnly();
	
	RefreshRoomHost();
}
