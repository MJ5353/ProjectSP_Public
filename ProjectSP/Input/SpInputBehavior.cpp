#include "SpInputBehavior.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputDefinition.h"

// ==================================================

ESpInputBehaviorRequest USpInputBehavior::HandleInput(ASpPlayerUnit& Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FSpInputActionBinding& Binding, bool bIsActiveBehavior)
{
	return K2_HandleInput(&Unit, Value, TriggerEvent, Binding.InputTag, Binding.AbilityTag, bIsActiveBehavior);
}

ESpInputBehaviorRequest USpInputBehavior::K2_HandleInput_Implementation(ASpPlayerUnit* Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FGameplayTag& InputTag, const FGameplayTag& AbilityTag, bool bIsActiveBehavior)
{
	return ESpInputBehaviorRequest::None;
}
