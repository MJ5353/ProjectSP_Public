#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpMapDefinition.generated.h"

class USpUnitDefinition;

// ==================================================

USTRUCT(BlueprintType)
struct FSpawnGroupData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	FTransform SpawnTransform = FTransform::Identity;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	float MaxRadius =0.f;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	uint8 Count = 1;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpUnitDefinition> UnitDefinition = nullptr;
};

// ==================================================

UCLASS()
class PROJECTSP_API USpMapDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TArray<FSpawnGroupData> EnemySpawnGroups;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TArray<FSpawnGroupData> OtherSpawnGroups;
};
