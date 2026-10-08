#include "SpGamePhaseStartingState.h"
#include "SpGamePhasePlayingState.h"
#include "SpGamePhaseLobbyState.h"
#include "ProjectSP/Common/SpLog.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/Component/SpUnitServerGatewayComponent.h"

// ==================================================

FSpGamePhaseStartingState::FSpGamePhaseStartingState(ASpGameMode& InGameMode) : FSpGamePhaseState(InGameMode)
{
}

// core

void FSpGamePhaseStartingState::Enter()
{
	// 시도마다 새 식별자를 부여해 이전 시도의 늦은 완료 보고를 무시한다.
	if (++GameMode.InitialPresentationId == 0)
		++GameMode.InitialPresentationId;

	GameState.bRoomStartAvailable = false;
	GameState.OnRep_RoomStartAvailable();

	FSpGamePhaseState::Enter();

	bPreparationFailed = !PreparePlayerUnits();
	if (bPreparationFailed)
		return;

	TArray<uint32> UnitUids;
	const auto AppendUnitUids = [&UnitUids](const TArray<TWeakObjectPtr<ASpUnit>>& Units)
	{
		for (const TWeakObjectPtr<ASpUnit>& Unit : Units)
		{
			if (Unit.IsValid() && Unit->HasValidUnitData())
				UnitUids.AddUnique(Unit->GetUnitUid());
		}
	};

	AppendUnitUids(GameMode.MapUnits);
	AppendUnitUids(GameMode.InitialPlayerUnits);

	bPreparationFailed = UnitUids.IsEmpty();
	if (bPreparationFailed)
		return;

	GameState.InitialPresentationId = GameMode.InitialPresentationId;
	GameState.InitialPresentationUnitUids = MoveTemp(UnitUids);

	GameState.OnRep_InitialPresentation();
	GameState.ForceNetUpdate();

	// 타이머는 만료 사실만 기록한다. 관전 전환과 phase 전환은 Update에서 처리한다.
	const FTimerDelegate OnTimeout = FTimerDelegate::CreateWeakLambda(&GameMode, [this]()
	{
		bTimedOut = true;
	});

	GameMode.GetWorldTimerManager().SetTimer(ReadyTimeoutHandle, OnTimeout, GameMode.ReadyTimeoutSeconds, false);
}

void FSpGamePhaseStartingState::Exit()
{
	GameMode.GetWorldTimerManager().ClearTimer(ReadyTimeoutHandle);
}

TUniquePtr<ISpState<ESpGamePhase>> FSpGamePhaseStartingState::Update()
{
	if (bRoomResetPending || bPreparationFailed)
		return MakeUnique<FSpGamePhaseLobbyState>(GameMode, bRoomResetPending);

	if (bTimedOut)
		RemoveUnreadyPlayers();

	bool bHasPlayer = false;
	for (const TWeakObjectPtr<ASpPlayerController>& Player : GameMode.InitialPlayers)
	{
		if (!Player.IsValid())
			continue;

		bHasPlayer = true;
		
		if (!ReadyPlayers.Contains(Player))
			return nullptr;
	}

	if (!bHasPlayer)
		return MakeUnique<FSpGamePhaseLobbyState>(GameMode);

	return MakeUnique<FSpGamePhasePlayingState>(GameMode);
}

// handle

void FSpGamePhaseStartingState::BeforePlayerLogout(ASpPlayerController& PlayerController)
{
	ReadyPlayers.Remove(&PlayerController);
	
	FSpGamePhaseState::BeforePlayerLogout(PlayerController);
}

void FSpGamePhaseStartingState::HandlePresentationReady(ASpPlayerController& PlayerController, const uint32 PresentationId)
{
	bool bFailedPresentation = bPreparationFailed || bTimedOut;
	bool bDiffPresentation = PresentationId != GameMode.InitialPresentationId;
	bool bInitialPlayer =!GameMode.InitialPlayers.Contains(&PlayerController);
	
	if (bFailedPresentation || bDiffPresentation || bInitialPlayer)
		return;
	
	ReadyPlayers.Add(&PlayerController);
}

bool FSpGamePhaseStartingState::PreparePlayerUnits()
{
	GameMode.InitialPlayerUnits.Reset();

	// WaitingToStart에서는 엔진이 Pawn을 만들지 않으므로 UID 목록을 확정하기 전에 생성한다.
	for (const TWeakObjectPtr<ASpPlayerController>& Player : GameMode.InitialPlayers)
	{
		ASpPlayerController* PlayerController = Player.Get();
		if (!PlayerController)
			continue;

		if (!PlayerController->GetPawn())
			GameMode.RestartPlayer(PlayerController);

		APawn* Pawn = PlayerController->GetPawn();
		ASpUnit* Unit = Cast<ASpUnit>(Pawn);
		if (!Unit || !Unit->HasValidUnitData())
		{
			if (Pawn)
			{
				PlayerController->UnPossess();
				Pawn->Destroy();
			}

			UE_LOG(LogMj, Warning, TEXT("플레이어 유닛 준비 실패: %s"), *GetNameSafe(PlayerController));
			return false;
		}

		GameMode.InitialPlayerUnits.AddUnique(Unit);
	}

	return !GameMode.InitialPlayerUnits.IsEmpty();
}

void FSpGamePhaseStartingState::RemoveUnreadyPlayers()
{
	TArray<TWeakObjectPtr<ASpPlayerController>> UnreadyPlayers;
	
	// unready player 수집
	for (const TWeakObjectPtr<ASpPlayerController>& Player : GameMode.InitialPlayers)
	{
		if (Player.IsValid() && !ReadyPlayers.Contains(Player))
			UnreadyPlayers.Add(Player);
	}

	// 삭제
	for (const TWeakObjectPtr<ASpPlayerController>& Player : UnreadyPlayers)
	{
		ASpPlayerController* PlayerController = Player.Get();
		if (!PlayerController)
			continue;

		ASpPlayerUnit* Unit = PlayerController->GetPawn<ASpPlayerUnit>();
		
		GameMode.InitialPlayerUnits.Remove(Unit);
		GameMode.InitialPlayers.Remove(Player);
		ReadyPlayers.Remove(Player);

		if (Unit && Unit->GetServerGateway())
			Unit->GetServerGateway()->ReturnUnit_Server();

		// 관전자로 전환
		PlayerController->StartSpectatingOnly();
	}
}
