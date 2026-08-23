#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SpGameMode.generated.h"

class USpMapDefinition;
class USpCameraDefinition;
class USpUnitDefinition;
class ASpGameState;
class ASpPlayerController;
class ASpUnit;

// ==================================================

UCLASS()
class PROJECTSP_API ASpGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	// [mj] todo) 임시 !!! 나중에 World Settings 기반 설정 검토
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpMapDefinition> MapDefinition;

	// 준비 완료 보고를 기다리는 최대 시간
	UPROPERTY(EditDefaultsOnly, Category="MJ - Dedicated Server", meta=(ClampMin="0.1"))
	float ReadyTimeoutSeconds = 5.0f;

private:
	bool bMapUnitsPrepared = false;
	bool bInitialPresentationSealed = false;	// 확정 여부 bool
	bool bInitialPresentationComplete = false;	// 끝났는지
	
	uint32 InitialPresentationId = 1;
	FTimerHandle ReadyTimeoutHandle;
	
	TArray<TWeakObjectPtr<ASpUnit>> InitialUnits;
	
	// player
	TArray<TWeakObjectPtr<ASpPlayerController>> ConnectedPlayers;
	TWeakObjectPtr<ASpPlayerController> RoomHostPlayer;

	// fixed player
	TSet<TWeakObjectPtr<ASpPlayerController>> InitialPlayers;
	TSet<TWeakObjectPtr<ASpPlayerController>> ReadyInitialPlayers;

public:	
	ASpGameMode();
	
	virtual void StartPlay() override;
	virtual void Logout(AController* Exiting) override;
	
	// override framework
	virtual bool ReadyToStartMatch_Implementation() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	virtual APawn* SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform) override;
	
	// callback
	void HandlePlayerReadyChanged_Server(ASpPlayerController* PlayerController, bool bReadyToStart);
	void HandleRoomStartRequest_Server(ASpPlayerController* PlayerController);
	void HandleInitialPresentationReady_Server(ASpPlayerController* PlayerController, uint32 InitialPresentationId);

private:
	// setting
	void PrepareMapUnits_Server();
	void RegisterInitialUnit_Server(ASpUnit* Unit);
	void RegisterConnectedPlayer_Server(ASpPlayerController* PlayerController);
	void SetRoomHost_Server(ASpPlayerController* PlayerController);
	void RemoveConnectedPlayer_Server(ASpPlayerController* PlayerController);
	
	// refresh
	void RefreshRoomHost_Server();
	void RefreshRoomStartAvailability_Server();
	bool AreAllInitialPlayersReadyToStart_Server() const;
	
	// presentation
	void TryStartInitialPresentation_Server(ASpPlayerController* RequestingPlayer);
	bool PrepareInitialPlayerUnits_Server();
	void BeginInitialPresentation_Server();
	void TryCompleteInitialPresentation_Server();
	void HandleInitialPresentationTimeout_Server();
	void CompleteInitialPresentation_Server();
	
	// exception
	void AbortInitialPresentation_Server();
	void ResetRoomWhenEmpty_Server();
	
	// get
	ASpGameState* GetSpGameState() const;
};
