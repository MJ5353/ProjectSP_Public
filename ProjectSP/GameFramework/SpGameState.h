#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "SpGameState.generated.h"

class ASpPlayerState;

// ==================================================

UENUM(BlueprintType)
enum class ESpGamePhase : uint8
{
	// 로비에서 플레이어의 Ready와 방장 Start 요청을 기다림
	WaitingForPlayers,

	// 초기 유닛 표현을 준비
	Preparing, 
	
	// 입력과 AI를 시작
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

public:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_GamePhase, Category = "MJ | Replicate")
	ESpGamePhase GamePhase = ESpGamePhase::WaitingForPlayers;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RoomHostPlayerState, Category = "MJ | Replicate")
	TObjectPtr<ASpPlayerState> RoomHostPlayerState; // 서버가 정한 방장

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RoomStartAvailable, Category = "MJ | Replicate")
	bool bRoomStartAvailable = false; // 로비 참여자 전원이 ready인지

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_InitialPresentation, Category = "MJ | Replicate")
	uint32 InitialPresentationId = 0; // 준비 요청을 구분하는 번호

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_InitialPresentation, Category = "MJ | Replicate")
	TArray<uint32> InitialPresentationUnitUids;

	UPROPERTY(Transient, BlueprintAssignable, Category = "MJ - Runtime")
	FSpLobbyStateChanged OnLobbyStateChanged;

	UPROPERTY(Transient, BlueprintAssignable, Category = "MJ - Runtime")
	FSpGamePhaseChanged OnGamePhaseChanged;
	
	// 클라이언트에서 표현 준비를 끝낸 UID만 기록
	TSet<uint32> PreparedUnitUids_ClientOnly;
	
	// 같은 InitialPresentationId에 대해 준비 완료 RPC를 한 번만 보내기 위한 보호 장치
	bool bPresentationReadyReported = false;

	// ------------------------------------------------
	
	ASpGameState();
	
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;

	// server
	void BeginInitialPresentation_Server(uint32 InInitialPresentationId, const TArray<uint32>& InInitialUnitUids);
	void ResetInitialPresentation_Server();
	void SetGamePhase_Server(ESpGamePhase InGamePhase);
	void SetRoomHostPlayerState_Server(class ASpPlayerState* InRoomHostPlayerState);
	void SetRoomStartAvailable_Server(bool bInRoomStartAvailable);

	// notify
	void NotifyLobbyStateChanged();
	void NotifyGamePhaseChanged();

	// client
	void NotifyUnitPresentationReady_ClientOnly(uint32 UnitUid);

	// get
	UFUNCTION(BlueprintCallable)
	ESpGamePhase GetGamePhase() const { return GamePhase; }

	UFUNCTION(BlueprintCallable)
	bool IsGameplayActive() const { return GamePhase == ESpGamePhase::Playing; }
	
	UFUNCTION(BlueprintCallable)
	bool IsRoomStartAvailable() const { return bRoomStartAvailable; }
	
	UFUNCTION(BlueprintCallable)
	bool IsRoomHost(const ASpPlayerState* PlayerState) const { return RoomHostPlayerState == PlayerState; }
	
private:
	void TryReportPresentationReady_ClientOnly();
	
	UFUNCTION()
	void OnRep_GamePhase();

	UFUNCTION()
	void OnRep_RoomHostPlayerState();

	UFUNCTION()
	void OnRep_RoomStartAvailable();

	UFUNCTION()
	void OnRep_InitialPresentation();
};
