#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpawnSubsystem.generated.h"

class USpMapDefinition;
class ASpUnit;
class ASpAIUnit;
class ASpPlayerUnit;
class USpUnitDefinition;
class USpUnitServerGatewayComponent;
struct FSpawnGroupData;
struct FActorSpawnParameters;

// ==================================================

UCLASS()
class PROJECTSP_API USpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	// uid
	uint32 NextUnitUid = 1;
	
	// maximum enemy
	static constexpr int32 MaxConcurrentEnemyUnits = 30;
	TSet<TWeakObjectPtr<ASpAIUnit>> ActiveEnemyUnits;
	
public:
	static USpawnSubsystem* Get(const UWorld* World);
	
	void SpawnMapUnits(const USpMapDefinition* MapDefinition, TArray<ASpUnit*>* OutSpawnedUnits = nullptr, bool bActivateImmediately = true);
	void ActivateUnit(ASpUnit* Unit, bool bKeepAlwaysRelevant = false);
	void ReleaseInitialRelevancy(ASpUnit* Unit);
	
	// spawn unit
	ASpAIUnit* SpawnAIUnit(USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParameters, const FGenericTeamId& TeamId, bool bActivateImmediately = true);
	ASpPlayerUnit* SpawnPlayerUnit(USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParameters, const FGenericTeamId& TeamId, bool bActivateImmediately = true);

private:
	// sync
	void PrepareSpawnedUnit_Server(ASpUnit* SpawnedUnit, const USpUnitServerGatewayComponent* ServerGateway);

	// get
	uint32 AllocateUnitUid();
	void GetSpreadLocation(const FSpawnGroupData& GroupData, TArray<FTransform>& SpawnTransforms);
};
