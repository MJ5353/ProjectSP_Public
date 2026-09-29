#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "ProjectSP/Common/StateMachine/SpStateMachine.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "SpGameMode.generated.h"

class USpMapDefinition;
class USpUnitDefinition;
class USpawnSubsystem;
class ASpPlayerController;
class ASpPlayerState;
class ASpPlayerUnit;
class ASpUnit;

// ==================================================

UCLASS()
class PROJECTSP_API ASpGameMode : public AGameMode
{
	GENERATED_BODY()

	friend class FSpGamePhaseState;
	friend class FSpGamePhaseLobbyState;
	friend class FSpGamePhaseStartingState;
	friend class FSpGamePhasePlayingState;

public:
	// [mj] todo) 나중에 World Settings 기반 설정 검토
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpMapDefinition> MapDefinition;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Dedicated Server", meta=(ClampMin="0.1"))
	float ReadyTimeoutSeconds = 5.0f;

private:
	ESpGamePhase Phase = ESpGamePhase::Lobby;
	TSpStateMachine<ESpGamePhase> PhaseMachine { Phase };

	bool bMapUnitsPrepared = false;
	uint32 InitialPresentationId = 0;
	
	TArray<TWeakObjectPtr<ASpUnit>> MapUnits;
	
	// player, player units
	TWeakObjectPtr<ASpPlayerController> RoomHostPlayer;
	TArray<TWeakObjectPtr<ASpPlayerController>> ConnectedPlayers;
	TSet<TWeakObjectPtr<ASpPlayerController>> InitialPlayers;
	TArray<TWeakObjectPtr<ASpUnit>> InitialPlayerUnits;

public:
	ASpGameMode();

	virtual void InitGameState() override;
	virtual void StartPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Logout(AController* Exiting) override;

	// override gameMode
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;

	// handle
	void HandlePlayerReadyChanged_Server(ASpPlayerController* PlayerController, bool bReadyToStart);
	void HandleRoomStartRequest_Server(ASpPlayerController* PlayerController);
	void HandleInitialPresentationReady_Server(ASpPlayerController* PlayerController, uint32 InInitialPresentationId);
	bool HandlePlayableUnitSelection_Server(ASpPlayerController* PlayerController, USpUnitDefinition* UnitDefinition);

private:
	bool ReplacePlayableUnit_Server(ASpPlayerController& PlayerController, ASpPlayerState& PlayerState, USpUnitDefinition* UnitDefinition);
	ASpPlayerUnit* SpawnReplacementUnit_Server(USpawnSubsystem& SpawnSubsystem, const ASpPlayerUnit& PreviousUnit, USpUnitDefinition* UnitDefinition);
};
