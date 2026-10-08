#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "Subsystems/WorldSubsystem.h"
#include "UnitRegistrySubsystem.generated.h"

struct FGenericTeamId;
struct FCollisionQueryParams;
class ASpUnit;

// ==================================================

UCLASS()
class PROJECTSP_API UUnitRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
	TMap<uint8, TArray<TObjectPtr<ASpUnit>>> UnitsByTeam;
	TArray<TObjectPtr<ASpUnit>> Units;
	
public:
	static UUnitRegistrySubsystem* Get(const UWorld* World);

private:
	// clear
	virtual void OnWorldEndPlay(UWorld& InWorld) override;
	void Clear();

	// core ------------------------------------------------

public:
	void RegisterUnit(ASpUnit* Unit);
	void UnregisterUnit(ASpUnit* Unit);
	void RefreshUnitTeam(ASpUnit* Unit);

private:
	void AddUnitToTeam(ASpUnit* Unit);
	void RemoveUnitFromTeamLists(ASpUnit* Unit);

public:
	// get ------------------------------------------------

	UFUNCTION(BlueprintCallable)
	ASpUnit* FindNearestUnit(const AActor* Querier, float Radius, ETeamAttitude::Type Attitude);

	void GetUnitsInRange(const FVector& Center, float Radius, TArray<ASpUnit*>& OutUnits, bool bIncludeCollisionRadius = false) const;
	bool IsUnitRegistered(const ASpUnit* Unit) const;
	
	UFUNCTION(BlueprintCallable)
	void GetTeamUnitsToIgnoreCollision(FGenericTeamId TeamId, TArray<AActor*>& OutActorsToIgnore) const;
	void GetTeamUnitsToIgnoreCollision(const FGenericTeamId& TeamId, FCollisionQueryParams& Params) const;
};
