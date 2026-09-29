#pragma once

#include "SpGamePhaseState.h"

// ==================================================

class FSpGamePhaseLobbyState final : public FSpGamePhaseState
{
	TWeakObjectPtr<ASpPlayerController> StartRequestPlayer;
	bool bResetRoom;

public:
	explicit FSpGamePhaseLobbyState(ASpGameMode& InGameMode, bool bInResetRoom = false);

	virtual ESpGamePhase GetState() const override { return ESpGamePhase::Lobby; }
	virtual bool CanChangeTo(ESpGamePhase NextState) const override { return NextState == ESpGamePhase::Starting; }
	
	// core
	virtual void Enter() override;
	virtual void StartPlay() override;
	virtual TUniquePtr<ISpState<ESpGamePhase>> Update() override;
	
	// logout
	virtual void AfterPlayerLogout() override;
	
	// joined
	virtual bool BeforePlayerJoin(ASpPlayerController& PlayerController) override;
	
	// handle
	virtual void HandlePlayerJoined(ASpPlayerController& PlayerController) override;
	virtual void HandlePlayerReadyChanged(ASpPlayerController& PlayerController, bool bReadyToStart) override;
	virtual void HandleStartRequested(ASpPlayerController& PlayerController) override;

private:
	void PrepareMapUnits();
	void ResetEmptyRoom();
	bool CanStart() const;
	void RefreshStartAvailability();
};
