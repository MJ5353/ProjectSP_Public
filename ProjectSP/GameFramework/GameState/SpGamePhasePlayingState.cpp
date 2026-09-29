#include "SpGamePhasePlayingState.h"
#include "SpGamePhaseLobbyState.h"
#include "EngineUtils.h"
#include "ProjectSP/Ability/Extensions/SpAbilityExtensionComponent.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpMonsterSpawner.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Subsystem/SpawnSubsystem.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

namespace
{
	constexpr float ExtensionPointIntervalSeconds = 10.0f;
}

// ------------------------------------------------

FSpGamePhasePlayingState::FSpGamePhasePlayingState(ASpGameMode& InGameMode) : FSpGamePhaseState(InGameMode)
{
}

void FSpGamePhasePlayingState::Enter()
{
	FSpGamePhaseState::Enter();

	if (USpawnSubsystem* SpawnSubsystem = USpawnSubsystem::Get(GameMode.GetWorld()))
	{
		// map unit에 설정해뒀던 relevancy 제거
		for (const TWeakObjectPtr<ASpUnit>& Unit : GameMode.MapUnits)
		{
			if (Unit.IsValid())
				SpawnSubsystem->ReleaseInitialRelevancy(Unit.Get());
		}

		for (const TWeakObjectPtr<ASpUnit>& Unit : GameMode.InitialPlayerUnits)
		{
			if (Unit.IsValid())
				SpawnSubsystem->ActivateUnit(Unit.Get());
		}
	}

	GameMode.StartMatch();

	const FTimerDelegate AwardPoints = FTimerDelegate::CreateWeakLambda(&GameMode, [this]()
	{
		AwardExtensionPoints();
	});
	GameMode.GetWorldTimerManager().SetTimer(ExtensionPointTimerHandle, AwardPoints, ExtensionPointIntervalSeconds, true);

	MonsterSpawners.Reset();
	for (TActorIterator<ASpMonsterSpawner> It(GameMode.GetWorld()); It; ++It)
	{
		ASpMonsterSpawner* Spawner = *It;
		MonsterSpawners.Add(TWeakObjectPtr<ASpMonsterSpawner>(Spawner));
		
		Spawner->StartSpawning();
	}
}

void FSpGamePhasePlayingState::Exit()
{
	GameMode.GetWorldTimerManager().ClearTimer(ExtensionPointTimerHandle);

	for (const TWeakObjectPtr<ASpMonsterSpawner>& Spawner : MonsterSpawners)
	{
		if (Spawner.IsValid())
			Spawner->StopSpawning();
	}
	MonsterSpawners.Reset();
}

TUniquePtr<ISpState<ESpGamePhase>> FSpGamePhasePlayingState::Update()
{
	if (bRoomResetPending)
		return MakeUnique<FSpGamePhaseLobbyState>(GameMode, true);
	
	return nullptr;
}

void FSpGamePhasePlayingState::AwardExtensionPoints()
{
	if (!GameMode.HasAuthority() || GameState.GamePhase != ESpGamePhase::Playing)
		return;

	for (const TWeakObjectPtr<ASpPlayerController>& Player : GameMode.InitialPlayers)
	{
		ASpPlayerController* PlayerController = Player.Get();
		if (!IsValid(PlayerController) || PlayerController->GetWorld() != GameMode.GetWorld())
			continue;

		if (USpAbilityExtensionComponent* Extensions = PlayerController->GetAbilityExtensionComponent())
			Extensions->GrantExtensionPoint_Server();
	}
}
