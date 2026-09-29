#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpInitialSettingDefinition.generated.h"

class USpUnitDefinition;

// ==================================================

UCLASS(BlueprintType)
class PROJECTSP_API USpInitialSettingDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting")
	TObjectPtr<USpUnitDefinition> UnitDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting")
	TArray<TObjectPtr<USpUnitDefinition>> PlayableUnitDefinitions;
};
