#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "SpMonsterSpawner.generated.h"

class USceneComponent;
class USpUnitDefinition;

// ==================================================

UCLASS()
class PROJECTSP_API ASpMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	TObjectPtr<USpUnitDefinition> UnitDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (ClampMin = "0.01"))
	float SpawnIntervalSeconds = 5.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	FTimerHandle SpawnTimerHandle;

public:
	ASpMonsterSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void SpawnMonster();

public:
	void StartSpawning();
	void StopSpawning();
};
