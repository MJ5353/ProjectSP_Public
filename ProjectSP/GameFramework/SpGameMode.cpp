#include "SpGameMode.h"
#include "SpGameState.h"
#include "SpPlayerController.h"
#include "SpPlayerState.h"
#include "ProjectSP/Common/SpLog.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Subsystem/SpawnSubsystem.h"
#include "ProjectSP/Unit/Define/SpTeam.h"

// ==================================================

ASpGameMode::ASpGameMode()
{
	GameStateClass = ASpGameState::StaticClass();
}

void ASpGameMode::StartPlay()
{
	Super::StartPlay();

	// 맵 세팅
	PrepareMapUnits_Server();

	// 입장 대기
	if (ASpGameState* SpGameState = GetSpGameState())
		SpGameState->SetGamePhase_Server(ESpGamePhase::WaitingForPlayers);
}

void ASpGameMode::Logout(AController* Exiting)
{
	if (ASpPlayerController* PlayerController = Cast<ASpPlayerController>(Exiting))
	{
		InitialPlayers.Remove(PlayerController);
		ReadyInitialPlayers.Remove(PlayerController);
		RemoveConnectedPlayer_Server(PlayerController);
	}

	Super::Logout(Exiting);

	if (ConnectedPlayers.IsEmpty())
	{
		ResetRoomWhenEmpty_Server();
		return;
	}

	// 방장이 나가면 방장 갱신
	RefreshRoomHost_Server();

	if (bInitialPresentationSealed && !bInitialPresentationComplete)
		TryCompleteInitialPresentation_Server();
	else if (!bInitialPresentationSealed)
		RefreshRoomStartAvailability_Server();
}

// override framework

bool ASpGameMode::ReadyToStartMatch_Implementation()
{
	// 모든 player의 initial unit load가 끝나야 매치 시작
	return bInitialPresentationComplete;
}

void ASpGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(NewPlayer);
	if (!PlayerController)
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}

	// Start 이후의 신규 접속 및 재접속은 게임에 합류시키지 않고 관전자로만 처리
	if (bInitialPresentationSealed)
	{
		RegisterConnectedPlayer_Server(PlayerController);
		NewPlayer->StartSpectatingOnly();

		return;
	}

	// 모두가 나간 경우를 대비해서 호출
	PrepareMapUnits_Server();
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	RegisterConnectedPlayer_Server(PlayerController);
	if (ASpPlayerState* PlayerState = PlayerController->GetPlayerState<ASpPlayerState>())
		PlayerState->SetReadyToStart_Server(false);

	InitialPlayers.Add(PlayerController);
	RegisterInitialUnit_Server(PlayerController->GetPawn<ASpUnit>());

	// Start 전에는 현재 입장자가 다음 게임의 참여 후보가 된다.
	RefreshRoomStartAvailability_Server();
}

UClass* ASpGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ASpPlayerState* PlayerState = InController->GetPlayerState<ASpPlayerState>())
	{
		if (USpUnitDefinition* UnitDefinition = PlayerState->UnitDefinition)
			return UnitDefinition->UnitClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ASpGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;

	const ASpPlayerController* PlayerController = Cast<ASpPlayerController>(NewPlayer);
	const ASpPlayerState* PlayerState = NewPlayer->GetPlayerState<ASpPlayerState>();

	if (!PlayerController || !PlayerState)
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

	if (USpUnitDefinition* UnitDefinition = PlayerState->UnitDefinition)
	{
		if (USpawnSubsystem* UnitSpawnSubsystem = GetWorld()->GetSubsystem<USpawnSubsystem>())
		{
			if (ASpUnit* SpawnedUnit = UnitSpawnSubsystem->SpawnUnit(UnitDefinition, SpawnTransform, SpawnInfo, FGenericTeamId(SpTeam::PlayerId), bInitialPresentationComplete))
			{
				// Initial Presentation 전에 확정된 참여자의 Pawn만 초기 대상에 등록한다.
				if (!bInitialPresentationSealed && InitialPlayers.Contains(PlayerController))
					RegisterInitialUnit_Server(SpawnedUnit);

				return SpawnedUnit;
			}
		}
	}
	
	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

// callback

void ASpGameMode::HandlePlayerReadyChanged_Server(ASpPlayerController* PlayerController, const bool bReadyToStart)
{
	if (!PlayerController || bInitialPresentationSealed || !InitialPlayers.Contains(PlayerController))
		return;

	if (ASpPlayerState* PlayerState = PlayerController->GetPlayerState<ASpPlayerState>())
		PlayerState->SetReadyToStart_Server(bReadyToStart);

	RefreshRoomStartAvailability_Server();
}

void ASpGameMode::HandleRoomStartRequest_Server(ASpPlayerController* PlayerController)
{
	TryStartInitialPresentation_Server(PlayerController);
}

void ASpGameMode::HandleInitialPresentationReady_Server(ASpPlayerController* PlayerController, const uint32 InInitialPresentationId)
{
	if (!PlayerController || 
		!bInitialPresentationSealed || bInitialPresentationComplete || // 확정되지 않았거나 이미 완료된 경우
		InInitialPresentationId != InitialPresentationId) // 식별자와 다른 경우
		return;

	// 기다릴 player가 아닌 경우
	if (!InitialPlayers.Contains(PlayerController))
		return;

	// ready list에 담기
	ReadyInitialPlayers.Add(PlayerController);

	TryCompleteInitialPresentation_Server();
}

// setting

void ASpGameMode::PrepareMapUnits_Server()
{
	if (bMapUnitsPrepared)
		return;

	if (!ensureMsgf(MapDefinition, TEXT("Map Definition이 링크되지 않음")))
		return;

	USpawnSubsystem* SpawnSubsystem = USpawnSubsystem::Get(GetWorld());
	if (!SpawnSubsystem)
		return;

	TArray<ASpUnit*> SpawnedMapUnits;
	SpawnSubsystem->SpawnMapUnits(MapDefinition, &SpawnedMapUnits, false);

	for (ASpUnit* Unit : SpawnedMapUnits)
		RegisterInitialUnit_Server(Unit);

	bMapUnitsPrepared = true;
}

void ASpGameMode::RegisterInitialUnit_Server(ASpUnit* Unit)
{
	if (!Unit)
		return;

	UE_LOG(LogMj, Log, TEXT("Register Unit: %s"), *Unit->GetName());
	InitialUnits.AddUnique(Unit);
}

void ASpGameMode::RegisterConnectedPlayer_Server(ASpPlayerController* PlayerController)
{
	if (!PlayerController)
		return;

	ConnectedPlayers.AddUnique(PlayerController);
	RefreshRoomHost_Server();
}

void ASpGameMode::SetRoomHost_Server(ASpPlayerController* PlayerController)
{
	if (!PlayerController)
		return;
	
	RoomHostPlayer = PlayerController;

	if (ASpGameState* SpGameState = GetSpGameState())
	{
		if (ASpPlayerState* SpPlayerState = PlayerController->GetPlayerState<ASpPlayerState>())
			SpGameState->SetRoomHostPlayerState_Server(SpPlayerState);
	}
}

void ASpGameMode::RemoveConnectedPlayer_Server(ASpPlayerController* PlayerController)
{
	ConnectedPlayers.RemoveAll([PlayerController](const TWeakObjectPtr<ASpPlayerController>& ConnectedPlayer)
	{
		return !ConnectedPlayer.IsValid() || ConnectedPlayer.Get() == PlayerController;
	});
}

// refresh

void ASpGameMode::RefreshRoomHost_Server()
{
	ConnectedPlayers.RemoveAll([](const TWeakObjectPtr<ASpPlayerController>& ConnectedPlayer)
	{
		return !ConnectedPlayer.IsValid();
	});

	const bool bCurrentHostConnected = ConnectedPlayers.ContainsByPredicate([this](const TWeakObjectPtr<ASpPlayerController>& ConnectedPlayer)
	{
		return ConnectedPlayer == RoomHostPlayer;
	});

	if (bCurrentHostConnected)
		return;

	ASpPlayerController* NextRoomHost = nullptr;
	for (const TWeakObjectPtr<ASpPlayerController>& ConnectedPlayer : ConnectedPlayers)
	{
		if (ConnectedPlayer.IsValid())
		{
			NextRoomHost = ConnectedPlayer.Get();
			break;
		}
	}

	SetRoomHost_Server(NextRoomHost);
}

void ASpGameMode::RefreshRoomStartAvailability_Server()
{
	ASpGameState* SpGameState = GetSpGameState();
	if (!SpGameState)
		return;

	const bool bCanStart = !bInitialPresentationSealed && !bInitialPresentationComplete && bMapUnitsPrepared && RoomHostPlayer.IsValid() && AreAllInitialPlayersReadyToStart_Server();

	SpGameState->SetRoomStartAvailable_Server(bCanStart);
}

bool ASpGameMode::AreAllInitialPlayersReadyToStart_Server() const
{
	bool bHasInitialPlayer = false;

	for (const TWeakObjectPtr<ASpPlayerController>& InitialPlayer : InitialPlayers)
	{
		const ASpPlayerController* PlayerController = InitialPlayer.Get();
		if (!PlayerController)
			continue;

		bHasInitialPlayer = true;

		const ASpPlayerState* PlayerState = PlayerController->GetPlayerState<ASpPlayerState>();
		if (!PlayerState || !PlayerState->bReadyToStart)
			return false;
	}

	return bHasInitialPlayer;
}

// presentation

void ASpGameMode::TryStartInitialPresentation_Server(ASpPlayerController* RequestingPlayer)
{
	// 방장이 요청했을 경우에만 시작
	if (!RequestingPlayer || RequestingPlayer != RoomHostPlayer)
		return;

	// 이미 presentation이 확정되었거나, 완료되었거나, 혹은 맵 유닛 세팅이 덜 되면 경우 return
	if (bInitialPresentationSealed || bInitialPresentationComplete || !bMapUnitsPrepared)
		return;

	if (!AreAllInitialPlayersReadyToStart_Server())
		return;

	// WaitingToStart에서는 엔진이 Pawn을 만들지 않는다. sealed 전에 초기 참여자의
	// Pawn/UnitData를 모두 준비해야 해당 UID가 InitialPresentation 목록에 포함된다.
	if (!PrepareInitialPlayerUnits_Server())
		return;

	BeginInitialPresentation_Server();
}

bool ASpGameMode::PrepareInitialPlayerUnits_Server()
{
	for (const TWeakObjectPtr<ASpPlayerController>& InitialPlayer : InitialPlayers)
	{
		ASpPlayerController* PlayerController = InitialPlayer.Get();
		if (!PlayerController)
			continue;

		if (!PlayerController->GetPawn())
			RestartPlayer(PlayerController);

		ASpUnit* PlayerUnit = PlayerController->GetPawn<ASpUnit>();
		if (!PlayerUnit || !PlayerUnit->HasValidUnitData())
		{
			UE_LOG(LogMj, Warning, TEXT("Initial Presentation Pawn preparation failed: %s"), *GetNameSafe(PlayerController));
			return false;
		}

		RegisterInitialUnit_Server(PlayerUnit);
	}

	return true;
}

void ASpGameMode::BeginInitialPresentation_Server()
{
	ASpGameState* SpGameState = GetSpGameState();
	if (!SpGameState)
		return;

	TArray<uint32> InitialUnitUids;
	for (const TWeakObjectPtr<ASpUnit>& Unit : InitialUnits)
	{
		if (Unit.IsValid() && Unit->HasValidUnitData())
			InitialUnitUids.Add(Unit->GetUnitUid());
	}

	if (InitialUnitUids.IsEmpty())
		return;

	// 이 시점부터 준비 대상과 참여 플레이어를 고정한다.
	// 이후 접속한 플레이어는 기존 초기 표현 절차에 참여하지 않는다.

	bInitialPresentationSealed = true;

	RefreshRoomStartAvailability_Server();
	SpGameState->BeginInitialPresentation_Server(InitialPresentationId, InitialUnitUids);

	// 5초(ready timeout) 이후에도 로드되지 않은 경우를 위한 binding
	GetWorldTimerManager().SetTimer(ReadyTimeoutHandle, this, 
		&ThisClass::HandleInitialPresentationTimeout_Server, ReadyTimeoutSeconds, false);
}

void ASpGameMode::TryCompleteInitialPresentation_Server()
{
	if (!bInitialPresentationSealed || bInitialPresentationComplete)
		return;

	bool bHasInitialPlayer = false;
	for (const TWeakObjectPtr<ASpPlayerController>& InitialPlayer : InitialPlayers)
	{
		if (!InitialPlayer.IsValid())
			continue;

		bHasInitialPlayer = true;
		if (!ReadyInitialPlayers.Contains(InitialPlayer))
			return;
	}

	if (!bHasInitialPlayer)
	{
		AbortInitialPresentation_Server();
		return;
	}

	CompleteInitialPresentation_Server();
}

void ASpGameMode::HandleInitialPresentationTimeout_Server()
{
	if (!bInitialPresentationSealed || bInitialPresentationComplete)
		return;

	TArray<ASpPlayerController*> UnreadyPlayers;
	for (const TWeakObjectPtr<ASpPlayerController>& InitialPlayer : InitialPlayers)
	{
		ASpPlayerController* PlayerController = InitialPlayer.Get();
		if (!PlayerController || ReadyInitialPlayers.Contains(PlayerController))
			continue;

		UnreadyPlayers.Add(PlayerController);
	}

	for (ASpPlayerController* PlayerController : UnreadyPlayers)
	{
		if (!PlayerController)
			continue;

		InitialPlayers.Remove(PlayerController);
		ReadyInitialPlayers.Remove(PlayerController);

		// 준비 시간을 넘긴 초기 참여자는 연결을 끊지 않고 관전자로 전환
		PlayerController->StartSpectatingOnly();
	}

	// 관전 전환 뒤 남은 초기 참여자의 완료 여부를 다시 판정한다
	TryCompleteInitialPresentation_Server();
}

void ASpGameMode::CompleteInitialPresentation_Server()
{
	// 참여자가 전혀 없으면 Playing으로 전환하지 않고 대기 상태로 되돌림
	if (bInitialPresentationComplete)
		return;

	bool bHasInitialPlayer = false;
	for (const TWeakObjectPtr<ASpPlayerController>& InitialPlayer : InitialPlayers)
	{
		if (InitialPlayer.IsValid())
		{
			bHasInitialPlayer = true;
			break;
		}
	}

	if (!bHasInitialPlayer)
	{
		AbortInitialPresentation_Server();
		return;
	}

	UE_LOG(LogMj, Log, TEXT("InitialUnits: %d"), InitialUnits.Num());
	
	bInitialPresentationComplete = true;
	GetWorldTimerManager().ClearTimer(ReadyTimeoutHandle);

	if (ASpGameState* SpGameState = GetSpGameState())
		SpGameState->SetGamePhase_Server(ESpGamePhase::Playing);

	if (USpawnSubsystem* SpawnSubsystem = GetWorld()->GetSubsystem<USpawnSubsystem>())
	{
		for (const TWeakObjectPtr<ASpUnit>& Unit : InitialUnits)
		{
			// spawn된 unit들을 활성화
			if (Unit.IsValid())
				SpawnSubsystem->ActivateUnit(Unit.Get());
		}
	}

	StartMatch();
}

// exception

void ASpGameMode::AbortInitialPresentation_Server()
{
	// 시작 대상이 모두 이탈하면 관전자가 남아 있더라도 Playing으로 진행하지 않는다.
	if (ConnectedPlayers.IsEmpty())
	{
		ResetRoomWhenEmpty_Server();
		return;
	}

	GetWorldTimerManager().ClearTimer(ReadyTimeoutHandle);
	bInitialPresentationComplete = false;
	ReadyInitialPlayers.Reset();

	if (ASpGameState* SpGameState = GetSpGameState())
		SpGameState->SetGamePhase_Server(ESpGamePhase::WaitingForPlayers);
}

void ASpGameMode::ResetRoomWhenEmpty_Server()
{
	// 마지막 플레이어가 나가면 게임 진행 상태를 유지하지 않고 다음 방을 위한 대기 상태로 초기화한다.
	if (!ConnectedPlayers.IsEmpty())
		return;

	GetWorldTimerManager().ClearTimer(ReadyTimeoutHandle);

	for (const TWeakObjectPtr<ASpUnit>& Unit : InitialUnits)
	{
		if (Unit.IsValid())
			Unit->Push();
	}

	InitialUnits.Reset();
	InitialPlayers.Reset();
	ReadyInitialPlayers.Reset();

	bMapUnitsPrepared = false;
	bInitialPresentationSealed = false;
	bInitialPresentationComplete = false;

	if (++InitialPresentationId == 0)
		++InitialPresentationId;

	SetRoomHost_Server(nullptr);

	if (ASpGameState* SpGameState = GetSpGameState())
		SpGameState->ResetInitialPresentation_Server();

	if (GetMatchState() == MatchState::InProgress)
		EndMatch();
	if (GetMatchState() != MatchState::WaitingToStart)
		SetMatchState(MatchState::WaitingToStart);
}

// get

ASpGameState* ASpGameMode::GetSpGameState() const
{
	return GetGameState<ASpGameState>();
}
