#pragma once

#include "ProjectSP/Common/StateMachine/SpStateMachine.h"
#include "ProjectSP/GameFramework/SpGameState.h"

class ASpGameMode;
class ASpPlayerController;

// ==================================================

// 서버에서만 존재한다. 공통 입퇴장 처리와 방장 선정도 이곳에서 직접 수행한다.

class FSpGamePhaseState : public ISpState<ESpGamePhase>
{
protected:
	ASpGameMode& GameMode;
	ASpGameState& GameState;
	bool bRoomResetPending = false;

	// ------------------------------------------------
	
	explicit FSpGamePhaseState(ASpGameMode& InGameMode);
	void RefreshRoomHost();
	
public:
	virtual void Enter() override;
	virtual void StartPlay() { }
	virtual TUniquePtr<ISpState<ESpGamePhase>> Update() { return nullptr; }
	
	// logout
	virtual void BeforePlayerLogout(ASpPlayerController& PlayerController);
	virtual void AfterPlayerLogout();

	// joined
	virtual bool BeforePlayerJoin(ASpPlayerController& PlayerController) { return false; }
	
	// handle
	virtual void HandlePlayerJoined(ASpPlayerController& PlayerController);
	virtual void HandlePlayerReadyChanged(ASpPlayerController& PlayerController, bool bReadyToStart) { }
	virtual void HandleStartRequested(ASpPlayerController& PlayerController) { }
	virtual void HandlePresentationReady(ASpPlayerController& PlayerController, uint32 PresentationId) { }
};
