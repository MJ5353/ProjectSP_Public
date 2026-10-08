#pragma once

#include "GameplayEffect.h"
#include "SpCooldownGameplayEffect.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpCooldownGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

public:
	USpCooldownGameplayEffect();

	static FGameplayTag GetDurationSetByCallerTag();
};
