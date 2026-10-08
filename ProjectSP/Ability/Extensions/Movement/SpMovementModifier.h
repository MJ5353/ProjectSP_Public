#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Ability/Extensions/SpAbilityModifierBase.h"
#include "SpMovementModifier.generated.h"

class USpGameplayAbility;

// ==================================================

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTSP_API USpMovementModifier : public USpAbilityModifierBase
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	float AdditionalDistance = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	float DistanceMultiplier = 1.0f;

public:
	virtual void AccumulateDistance(float& InOutAdditionalDistance, float& InOutMultiplier) const;
	virtual void OnMovementStarted(USpGameplayAbility* Ability, const FVector& StartLocation) {}
	virtual void OnMovementStep(USpGameplayAbility* Ability, const FVector& PreviousLocation, const FVector& CurrentLocation) {}
	virtual void OnMovementEnded(USpGameplayAbility* Ability, const FVector& EndLocation) {}
};
