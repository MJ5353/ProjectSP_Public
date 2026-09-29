#include "SpGameState.h"
#include "EngineUtils.h"
#include "SpPlayerController.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/Definition/Player/SpInitialSettingDefinition.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/Component/SpUnitClientGatewayComponent.h"

// ==================================================

ASpGameState::ASpGameState()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;
}

void ASpGameState::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (GetNetMode() == NM_DedicatedServer)
		return;
	
	// 복제된 준비 목록과 실제 로컬 유닛 로드 완료를 맞추는 클라이언트 처리
	if (bPresentationRefreshPending_Client)
	{
		bPresentationRefreshPending_Client = false;
		RequestPresentationReadyReports_Client();
	}

	TryRevealLobbyMapUnits_Client();
	TryReportPresentationReady_Client();
}

void ASpGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpGameState, GamePhase);
	DOREPLIFETIME(ASpGameState, RoomHostPlayerState);
	DOREPLIFETIME(ASpGameState, bRoomStartAvailable);
	DOREPLIFETIME(ASpGameState, InitialPresentationId);
	DOREPLIFETIME(ASpGameState, InitialPresentationUnitUids);
	DOREPLIFETIME(ASpGameState, LobbyMapPresentationUnitUids);
	DOREPLIFETIME(ASpGameState, bLobbyMapUnitsActivated);
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

// lobby state

void ASpGameState::NotifyLobbyStateChanged()
{
	OnLobbyStateChanged.Broadcast();
}

// presentation ready

void ASpGameState::NotifyUnitPresentationReady_Client(const uint32 UnitUid)
{
	if (GetNetMode() == NM_DedicatedServer || UnitUid == 0)
		return;

	PreparedUnitUids_Client.Add(UnitUid);
	if (GamePhase == ESpGamePhase::Playing)
	{
		if (bLobbyMapUnitsActivated && LobbyMapPresentationUnitUids.Contains(UnitUid))
		{
			for (TActorIterator<ASpUnit> It(GetWorld()); It; ++It)
			{
				if (It->GetUnitUid() == UnitUid)
				{
					It->SetUnitPresentationVisible_Client(true);
					break;
				}
			}
		}
		return;
	}

	TryRevealLobbyMapUnits_Client();
}

void ASpGameState::RemoveUnitPresentationReady_Client(const uint32 UnitUid)
{
	if (PreparedUnitUids_Client.Remove(UnitUid) == 0)
		return;

	if (InitialPresentationUnitUids.Contains(UnitUid))
		bPresentationReadyReported = false;

	if (LobbyMapPresentationUnitUids.Contains(UnitUid))
		bLobbyMapPresentationVisible_Client = false;
}

bool ASpGameState::IsPlayableUnitDefinition(const USpUnitDefinition* UnitDefinition) const
{
	if (!IsValid(UnitDefinition) || !InitialDefinition || !UnitDefinition->UnitClass || !UnitDefinition->UnitClass->IsChildOf(ASpPlayerUnit::StaticClass()))
		return false;

	for (const USpUnitDefinition* PlayableUnit : InitialDefinition->PlayableUnitDefinitions)
	{
		if (PlayableUnit == UnitDefinition)
			return true;
	}

	return false;
}

USpUnitDefinition* ASpGameState::GetDefaultUnitDefinition() const
{
	return InitialDefinition ? InitialDefinition->UnitDefinition.Get() : nullptr;
}

TArray<USpUnitDefinition*> ASpGameState::GetPlayableUnitDefinitions() const
{
	TArray<USpUnitDefinition*> PlayableUnits;
	if (InitialDefinition)
	{
		for (USpUnitDefinition* UnitDefinition : InitialDefinition->PlayableUnitDefinitions)
		{
			if (IsPlayableUnitDefinition(UnitDefinition))
				PlayableUnits.AddUnique(UnitDefinition);
		}
	}
	
	return PlayableUnits;
}

void ASpGameState::RequestPresentationReadyReports_Client()
{
	for (TActorIterator<ASpUnit> It(GetWorld()); It; ++It)
	{
		const uint32 UnitUid = It->GetUnitUid();
		
		if (!InitialPresentationUnitUids.Contains(UnitUid) && !LobbyMapPresentationUnitUids.Contains(UnitUid))
			continue;

		if (USpUnitClientGatewayComponent* ClientGateway = It->GetClientGateway())
			ClientGateway->ReportPresentationReadyToGameState_Client();
	}
}

void ASpGameState::TryReportPresentationReady_Client()
{
	if (GamePhase != ESpGamePhase::Starting || bPresentationReadyReported ||
		InitialPresentationId == 0 || InitialPresentationUnitUids.IsEmpty())
	{
		return;
	}

	for (const uint32 UnitUid : InitialPresentationUnitUids)
	{
		if (!PreparedUnitUids_Client.Contains(UnitUid))
			return;
	}

	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(GetWorld()->GetFirstPlayerController());
	if (!PlayerController || !PlayerController->IsLocalController())
		return;

	// Listen Server도 같은 RPC 경로를 사용한다. 서버 상태의 Enter 도중에는 보고하지 않는다.
	bPresentationReadyReported = true;
	PlayerController->ServerReportInitialPresentationReady(InitialPresentationId);
}

void ASpGameState::TryRevealLobbyMapUnits_Client()
{
	if (GamePhase == ESpGamePhase::Playing || bLobbyMapPresentationVisible_Client || !bLobbyMapUnitsActivated || LobbyMapPresentationUnitUids.IsEmpty())
		return;

	for (const uint32 UnitUid : LobbyMapPresentationUnitUids)
	{
		if (!PreparedUnitUids_Client.Contains(UnitUid))
			return;
	}

	bLobbyMapPresentationVisible_Client = true;
	
	for (TActorIterator<ASpUnit> It(GetWorld()); It; ++It)
	{
		if (LobbyMapPresentationUnitUids.Contains(It->GetUnitUid()))
			It->SetUnitPresentationVisible_Client(true);
	}
}

// rep notify

void ASpGameState::OnRep_GamePhase()
{
	bPresentationRefreshPending_Client = true;
	OnGamePhaseChanged.Broadcast();
}

void ASpGameState::OnRep_RoomHostPlayerState()
{
	NotifyLobbyStateChanged();
	ForceNetUpdate();
}

void ASpGameState::OnRep_RoomStartAvailable()
{
	NotifyLobbyStateChanged();
}

void ASpGameState::OnRep_InitialPresentation()
{
	bPresentationReadyReported = false;
	bPresentationRefreshPending_Client = true;
}

void ASpGameState::OnRep_LobbyMapPresentation()
{
	bLobbyMapPresentationVisible_Client = false;
	bPresentationRefreshPending_Client = true;
}
