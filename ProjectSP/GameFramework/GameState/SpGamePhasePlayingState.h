#pragma once

#include "SpGamePhaseState.h"
#include "TimerManager.h"

class ASpMonsterSpawner;

// ==================================================

class FSpGamePhasePlayingState final : public FSpGamePhaseState
{
	FTimerHandle ExtensionPointTimerHandle;
	TArray<TWeakObjectPtr<ASpMonsterSpawner>> MonsterSpawners;

public:
	explicit FSpGamePhasePlayingState(ASpGameMode& InGameMode);

	virtual ESpGamePhase GetState() const override { return ESpGamePhase::Playing; }
	virtual bool CanChangeTo(ESpGamePhase NextState) const override { return NextState == ESpGamePhase::Lobby; }
	
	virtual void Enter() override;
	virtual void Exit() override;
	virtual TUniquePtr<ISpState<ESpGamePhase>> Update() override;

private:
	void AwardExtensionPoints();
};
