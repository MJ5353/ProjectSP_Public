#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpCameraDefinition.generated.h"

class UCurveVector;

// ==================================================

UCLASS()
class PROJECTSP_API USpCameraDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category = "MJ - Setting", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMin = -89.9f;

	UPROPERTY(EditDefaultsOnly, Category = "MJ - Setting", Meta = (UIMin = "-89.9", UIMax = "89.9", ClampMin = "-89.9", ClampMax = "89.9"))
	float ViewPitchMax = 89.9f;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<const UCurveVector> TargetOffsetCurve;
};
