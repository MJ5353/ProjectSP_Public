#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpGameState.generated.h"

class ASpPlayerState;
class USpInitialSettingDefinition;
class USpUnitDefinition;
class FSpGamePhaseState;
class FSpGamePhaseLobbyState;
class FSpGamePhaseStartingState;
class FSpGamePhasePlayingState;

// ==================================================

UENUM(BlueprintType)
enum class ESpGamePhase : uint8
{
	// 맵 준비 및 플레이어 Ready / 방장 Start 대기
	Lobby,

	// Start 승인 후 유닛 준비 및 클라이언트 준비 완료 대기
	Starting,

	// 실제 플레이 진행
	Playing,
};

// ==================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSpLobbyStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSpGamePhaseChanged);

// ------------------------------------------------

UCLASS()
class PROJECTSP_API ASpGameState : public AGameState
{
	GENERATED_BODY()

	friend class FSpGamePhaseState;
	friend class FSpGamePhaseLobbyState;
	friend class FSpGamePhaseStartingState;
	friend class FSpGamePhasePlayingState;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	TObjectPtr<USpInitialSettingDefinition> InitialDefinition;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GamePhase, Category = "MJ | Replicate")
	ESpGamePhase GamePhase = ESpGamePhase::Lobby;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RoomHostPlayerState, Category = "MJ | Replicate")
	TObjectPtr<ASpPlayerState> RoomHostPlayerState; // 서버가 정한 방장

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RoomStartAvailable, Category = "MJ | Replicate")
	bool bRoomStartAvailable = false; // 로비 참여자 전원이 ready인지

private:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_InitialPresentation, Category = "MJ | Replicate")
	uint32 InitialPresentationId = 0; // 준비 요청을 구분하는 번호

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_InitialPresentation, Category = "MJ | Replicate")
	TArray<uint32> InitialPresentationUnitUids;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_LobbyMapPresentation, Category = "MJ | Replicate")
	TArray<uint32> LobbyMapPresentationUnitUids;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_LobbyMapPresentation, Category = "MJ | Replicate")
	bool bLobbyMapUnitsActivated = false;

	UPROPERTY(Transient, BlueprintAssignable, Category = "MJ - Runtime")
	FSpLobbyStateChanged OnLobbyStateChanged;

	UPROPERTY(Transient, BlueprintAssignable, Category = "MJ - Runtime")
	FSpGamePhaseChanged OnGamePhaseChanged;

	TSet<uint32> PreparedUnitUids_Client;
	bool bPresentationReadyReported = false;
	bool bLobbyMapPresentationVisible_Client = false;
	bool bPresentationRefreshPending_Client = true;

public:
	ASpGameState();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	// lobby state
	void NotifyLobbyStateChanged();
	
	// presentation ready
	void NotifyUnitPresentationReady_Client(uint32 UnitUid);
	void RemoveUnitPresentationReady_Client(uint32 UnitUid);

	// get
	bool IsPlayableUnitDefinition(const USpUnitDefinition* UnitDefinition) const;

	UFUNCTION(BlueprintPure, Category = "MJ - Unit")
	USpUnitDefinition* GetDefaultUnitDefinition() const;

	UFUNCTION(BlueprintPure, Category = "MJ - Unit")
	TArray<USpUnitDefinition*> GetPlayableUnitDefinitions() const;

	UFUNCTION(BlueprintCallable)
	ESpGamePhase GetGamePhase() const { return GamePhase; }

	UFUNCTION(BlueprintCallable)
	bool IsGameplayActive() const { return GamePhase == ESpGamePhase::Playing; }

	UFUNCTION(BlueprintCallable)
	bool IsRoomHost(const ASpPlayerState* PlayerState) const { return RoomHostPlayerState == PlayerState; }

private:
	void RequestPresentationReadyReports_Client();
	void TryReportPresentationReady_Client();
	void TryRevealLobbyMapUnits_Client();

	UFUNCTION()
	void OnRep_GamePhase();

	UFUNCTION()
	void OnRep_RoomHostPlayerState();

	UFUNCTION()
	void OnRep_RoomStartAvailable();

	UFUNCTION()
	void OnRep_InitialPresentation();

	UFUNCTION()
	void OnRep_LobbyMapPresentation();
};
