#pragma once

#include "GameplayTagContainer.h"
#include "FSpAIAbilityOption.generated.h"

// ==================================================

USTRUCT(BlueprintType)
struct FSpAIAbilityOption
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	int32 Priority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	FGameplayTag AbilityTag;

};
