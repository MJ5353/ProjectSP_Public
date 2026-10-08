#pragma once

#include "SpGamePhaseState.h"
#include "TimerManager.h"

// ==================================================

class FSpGamePhaseStartingState final : public FSpGamePhaseState
{
	TSet<TWeakObjectPtr<ASpPlayerController>> ReadyPlayers;
	FTimerHandle ReadyTimeoutHandle;
	
	bool bPreparationFailed = false;
	bool bTimedOut = false;

public:
	explicit FSpGamePhaseStartingState(ASpGameMode& InGameMode);

	virtual ESpGamePhase GetState() const override { return ESpGamePhase::Starting; }
	virtual bool CanChangeTo(ESpGamePhase NextState) const override { return NextState == ESpGamePhase::Playing || NextState == ESpGamePhase::Lobby; }
	
	// core
	virtual void Enter() override;
	virtual void Exit() override;
	virtual TUniquePtr<ISpState<ESpGamePhase>> Update() override;
	
	// handle
	virtual void BeforePlayerLogout(ASpPlayerController& PlayerController) override;
	virtual void HandlePresentationReady(ASpPlayerController& PlayerController, uint32 PresentationId) override;

private:
	bool PreparePlayerUnits();
	void RemoveUnreadyPlayers();
};
