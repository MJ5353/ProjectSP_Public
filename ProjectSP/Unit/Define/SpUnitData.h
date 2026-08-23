#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "SpUnitData.generated.h"

class USpUnitDefinition;

// ==================================================

USTRUCT(BlueprintType)
struct FSpUnitData
{
	GENERATED_BODY()
	
	static constexpr uint32 InvalidUnitUid = 0;
	
	UPROPERTY(Transient, VisibleInstanceOnly, Category = "MJ - Runtime")
	uint32 UnitUid = InvalidUnitUid;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TObjectPtr<USpUnitDefinition> UnitDefinition;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	uint8 TeamId = FGenericTeamId::NoTeam.GetId();

	// ------------------------------------------------
	
	FSpUnitData() = default;
	
	FSpUnitData(uint32 UID, USpUnitDefinition* InUnitDefinition, FGenericTeamId InTeamId)
	{
		UnitUid = UID;
		UnitDefinition = InUnitDefinition;
		TeamId = InTeamId;
	}
	
	bool IsValid() const
	{
		return UnitUid != InvalidUnitUid && UnitDefinition != nullptr;
	}

	void Reset()
	{
		UnitUid = InvalidUnitUid;
		UnitDefinition = nullptr;
		TeamId = FGenericTeamId::NoTeam.GetId();
	}
};
