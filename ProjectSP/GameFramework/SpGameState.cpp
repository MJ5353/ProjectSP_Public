#include "SpGameState.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/GameFramework/SpPlayerState.h"

// ==================================================

ASpGameState::ASpGameState()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ASpGameState::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (!HasAuthority() && GamePhase == ESpGamePhase::Preparing && !bPresentationReadyReported)
		TryReportPresentationReady_ClientOnly();
}

void ASpGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpGameState, GamePhase);
	DOREPLIFETIME(ASpGameState, RoomHostPlayerState);
	DOREPLIFETIME(ASpGameState, bRoomStartAvailable);
	DOREPLIFETIME(ASpGameState, InitialPresentationId);
	DOREPLIFETIME(ASpGameState, InitialPresentationUnitUids);
}

void ASpGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	NotifyLobbyStateChanged();
}

void ASpGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	NotifyLobbyStateChanged();
}

// server

void ASpGameState::BeginInitialPresentation_Server(const uint32 InInitialPresentationId, const TArray<uint32>& InInitialUnitUids)
{
	check(HasAuthority());
	check(InInitialPresentationId != 0);

	InitialPresentationId = InInitialPresentationId;
	InitialPresentationUnitUids = InInitialUnitUids;

	SetGamePhase_Server(ESpGamePhase::Preparing);
	SetRoomStartAvailable_Server(false);
	
	// Listen Server의 호스트는 복제를 받지 않으므로
	// 이미 수집한 로컬 준비 상태를 같은 조건으로 평가한다.
	TryReportPresentationReady_ClientOnly();
}

void ASpGameState::ResetInitialPresentation_Server()
{
	check(HasAuthority());

	InitialPresentationId = 0;
	InitialPresentationUnitUids.Reset();
	
	SetGamePhase_Server(ESpGamePhase::WaitingForPlayers);
	SetRoomStartAvailable_Server(false);
}

void ASpGameState::SetGamePhase_Server(const ESpGamePhase InGamePhase)
{
	check(HasAuthority());
	GamePhase = InGamePhase;

	NotifyGamePhaseChanged();
	ForceNetUpdate();
}

void ASpGameState::SetRoomHostPlayerState_Server(ASpPlayerState* InRoomHostPlayerState)
{
	check(HasAuthority());

	if (RoomHostPlayerState == InRoomHostPlayerState)
		return;

	RoomHostPlayerState = InRoomHostPlayerState;
	NotifyLobbyStateChanged();
	ForceNetUpdate();
}

void ASpGameState::SetRoomStartAvailable_Server(const bool bInRoomStartAvailable)
{
	check(HasAuthority());

	if (bRoomStartAvailable == bInRoomStartAvailable)
		return;

	bRoomStartAvailable = bInRoomStartAvailable;
	NotifyLobbyStateChanged();
	ForceNetUpdate();
}

// notify

void ASpGameState::NotifyLobbyStateChanged()
{
	OnLobbyStateChanged.Broadcast();
}

void ASpGameState::NotifyGamePhaseChanged()
{
	OnGamePhaseChanged.Broadcast();
}

// client

void ASpGameState::NotifyUnitPresentationReady_ClientOnly(const uint32 UnitUid)
{
	if (GetNetMode() == NM_DedicatedServer || UnitUid == 0)
		return;

	PreparedUnitUids_ClientOnly.Add(UnitUid);
	TryReportPresentationReady_ClientOnly();
}

// private

void ASpGameState::TryReportPresentationReady_ClientOnly()
{
	// 클라이언트가 초기 유닛의 prepare를 끝냈음을 서버에 보내는 함수
	
	if (bPresentationReadyReported || GamePhase != ESpGamePhase::Preparing || InitialPresentationId == 0)
		return;

	if (HasAuthority() && GetNetMode() != NM_ListenServer && GetNetMode() != NM_Standalone)
		return;

	// 아직 복제할 목록을 받지 못한 경우 return
	if (InitialPresentationUnitUids.IsEmpty())
		return;

	// 표현 준비가 하나라도 덜 끝난 경우 return
	for (const uint32 UnitUid : InitialPresentationUnitUids)
	{
		if (!PreparedUnitUids_ClientOnly.Contains(UnitUid))
			return;
	}

	if (ASpPlayerController* PlayerController = Cast<ASpPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		bPresentationReadyReported = true;

		if (HasAuthority()) // listen server 인 경우
		{
			if (ASpGameMode* GameMode = GetWorld()->GetAuthGameMode<ASpGameMode>())
				GameMode->HandleInitialPresentationReady_Server(PlayerController, InitialPresentationId);
		}
		else
		{
			PlayerController->ServerReportInitialPresentationReady(InitialPresentationId);
		}
	}
}

void ASpGameState::OnRep_GamePhase()
{
	TryReportPresentationReady_ClientOnly();
	NotifyGamePhaseChanged();
}

void ASpGameState::OnRep_RoomHostPlayerState()
{
	NotifyLobbyStateChanged();
}

void ASpGameState::OnRep_RoomStartAvailable()
{
	NotifyLobbyStateChanged();
}

void ASpGameState::OnRep_InitialPresentation()
{
	bPresentationReadyReported = false;
	TryReportPresentationReady_ClientOnly();
}
