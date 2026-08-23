#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpawnSubsystem.generated.h"

class USpMapDefinition;
class ASpUnit;
class USpUnitDefinition;
struct FSpawnGroupData;
struct FActorSpawnParameters;

// ==================================================

UCLASS()
class PROJECTSP_API USpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	// uid
	uint32 NextUnitUid = 1;
	
public:
	static USpawnSubsystem* Get(const UWorld* World);
	
	void SpawnMapUnits(const USpMapDefinition* MapDefinition, TArray<ASpUnit*>* OutSpawnedUnits = nullptr, bool bActivateImmediately = true);
	ASpUnit* SpawnUnit(USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParameters, const FGenericTeamId& TeamId, bool bActivateImmediately = true);
	
	void ActivateUnit(ASpUnit* Unit);

private:
	// get
	uint32 AllocateUnitUid();
	void GetSpreadLocation(const FSpawnGroupData& GroupData, TArray<FTransform>& SpawnTransforms);
};
