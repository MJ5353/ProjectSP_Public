#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Ability/Extensions/Movement/SpMovementModifier.h"
#include "SpAreaTrailModifier.generated.h"

class AActor;

// ==================================================

UCLASS(EditInlineNew, meta=(DisplayName="Area Trail Modifier"))
class PROJECTSP_API USpAreaTrailModifier : public USpMovementModifier
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category="MJ - Ability Modifier|Area Trail")
	TSubclassOf<AActor> DamageAreaClass;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Ability Modifier|Area Trail", meta=(ClampMin="1.0"))
	float AreaSpacing = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Ability Modifier|Area Trail")
	bool bPlaceAtStart = true;

private:
	float DistanceTraveled = 0.0f;
	float NextPlacementDistance = 0.0f;

public:
	virtual void OnMovementStarted(USpGameplayAbility* Ability, const FVector& StartLocation) override;
	virtual void OnMovementStep(USpGameplayAbility* Ability, const FVector& PreviousLocation, const FVector& CurrentLocation) override;

private:
	void PlaceArea(USpGameplayAbility* Ability, const FVector& Location) const;
};
