#include "SpMonsterSpawner.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "ProjectSP/Common/SpLog.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/Subsystem/SpawnSubsystem.h"
#include "ProjectSP/Unit/SpAIUnit.h"
#include "ProjectSP/Unit/Define/SpTeam.h"

// ==================================================

ASpMonsterSpawner::ASpMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void ASpMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	const ASpGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpGameState>() : nullptr;
	if (GameState && GameState->IsGameplayActive())
		StartSpawning();
}

void ASpMonsterSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopSpawning();
	
	Super::EndPlay(EndPlayReason);
}

void ASpMonsterSpawner::SpawnMonster()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
		return;

	USpawnSubsystem* SpawnSubsystem = USpawnSubsystem::Get(World);
	if (!SpawnSubsystem)
		return;

	const FTransform SpawnTransform(GetActorQuat(), GetActorLocation() + FVector::UpVector * 50.f, FVector::OneVector);
	
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnSubsystem->SpawnAIUnit(UnitDefinition, SpawnTransform, SpawnParameters, FGenericTeamId(SpTeam::EnemyId));
}

void ASpMonsterSpawner::StartSpawning()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client || GetWorldTimerManager().IsTimerActive(SpawnTimerHandle))
		return;

	if (!UnitDefinition || !UnitDefinition->UnitClass || !UnitDefinition->UnitClass->IsChildOf(ASpAIUnit::StaticClass()))
	{
		UE_LOG(LogMj, Warning, TEXT("Monster spawner %s needs an AI UnitDefinition."), *GetName());
		return;
	}

	if (SpawnIntervalSeconds <= 0.0f)
	{
		UE_LOG(LogMj, Warning, TEXT("Monster spawner %s needs a positive spawn interval."), *GetName());
		return;
	}

	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ThisClass::SpawnMonster, SpawnIntervalSeconds, true, SpawnIntervalSeconds);
}

void ASpMonsterSpawner::StopSpawning()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(SpawnTimerHandle);
}
