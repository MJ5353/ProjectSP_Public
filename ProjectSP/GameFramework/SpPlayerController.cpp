#include "SpPlayerController.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/GameFramework/SpPlayerState.h"
#include "ProjectSP/Unit/Component/Player/SpPlayerCommandComponent.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

void ASpPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	if (IsGameplayInputEnabled_Client())
	{
		if (USpAbilitySystemComponent* ASC = GetSpAbilitySystemComponent())
			ASC->OnProcessAbilityInput(DeltaTime, bGamePaused);
	}
	
	Super::PostProcessInput(DeltaTime, bGamePaused);
}

// server

void ASpPlayerController::ServerSetReady_Implementation(const bool bReadyToStart)
{
	// GameState가 Playing일 때만 로컬 게임플레이 입력을 허용
	
	if (UWorld* World = GetWorld())
	{
		if (ASpGameMode* GameMode = World->GetAuthGameMode<ASpGameMode>())
			GameMode->HandlePlayerReadyChanged_Server(this, bReadyToStart);
	}
}

void ASpPlayerController::ServerRequestStart_Implementation()
{
	// 현재 방장만 요청할 수 있으며, 서버가 전원 Ready 여부를 다시 검증한다.

	if (UWorld* World = GetWorld())
	{
		if (ASpGameMode* GameMode = World->GetAuthGameMode<ASpGameMode>())
			GameMode->HandleRoomStartRequest_Server(this);
	}
}

void ASpPlayerController::ServerReportInitialPresentationReady_Implementation(const uint32 InitialPresentationId)
{
	// 초기 유닛 표현 준비가 끝난 로컬 클라이언트가 서버에 보내는 완료 보고
	
	if (UWorld* World = GetWorld())
	{
		if (ASpGameMode* GameMode = World->GetAuthGameMode<ASpGameMode>())
			GameMode->HandleInitialPresentationReady_Server(this, InitialPresentationId);
	}
}

void ASpPlayerController::ClientSetCommandMoveState_Implementation(const uint16 CommandId, const bool bHasWaypoint, const FVector_NetQuantize10 Waypoint)
{
	if (ASpPlayerUnit* PlayerUnit = GetPawn<ASpPlayerUnit>())
	{
		if (USpPlayerCommandComponent* CommandComponent = PlayerUnit->GetPlayerCommandComponent())
			CommandComponent->ApplyMovementState_Client(CommandId, bHasWaypoint, Waypoint);
	}
}

// get

bool ASpPlayerController::IsGameplayInputEnabled_Client() const
{
	const UWorld* World = GetWorld();
	const ASpGameState* GameState = World ? World->GetGameState<ASpGameState>() : nullptr;
	
	// GameState가 Playing일 때만 로컬 게임플레이 입력을 허용
	return GameState && GameState->IsGameplayActive();
}

USpAbilitySystemComponent* ASpPlayerController::GetSpAbilitySystemComponent() const
{
	if (const ASpPlayerState* SpPlayerState = GetPlayerState<ASpPlayerState>())
		return SpPlayerState->GetSpAbilitySystemComponent();
	
	return nullptr;
}
