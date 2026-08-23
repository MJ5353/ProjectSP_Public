#include "SpGameplayAbility.h"

// ==================================================

USpGameplayAbility::USpGameplayAbility(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ActivationPolicy = EMjAbilityActivationPolicy::OnInputTriggered;
}
