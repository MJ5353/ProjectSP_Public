#include "SpGameplayEventInputBehavior.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputDefinition.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

ESpInputBehaviorRequest USpGameplayEventInputBehavior::HandleInput(ASpPlayerUnit& Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FSpInputActionBinding& Binding, bool bIsActiveBehavior)
{
	if (TriggerEvent != SendOnTriggerEvent || !Unit.IsLocallyControlled() || !Unit.GetSpAbilitySystemComponent() || !EventTag.IsValid())
		return ESpInputBehaviorRequest::None;

	FGameplayEventData Payload;
	if (!BuildEventPayload(&Unit, Value, Binding.InputTag, Binding.AbilityTag, Payload))
		return ESpInputBehaviorRequest::None;

	Payload.EventTag = EventTag;
	Payload.Instigator = &Unit;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(&Unit, EventTag, Payload);
	
	return ESpInputBehaviorRequest::None;
}

bool USpGameplayEventInputBehavior::BuildEventPayload_Implementation(ASpPlayerUnit* Unit, const FInputActionValue& Value, const FGameplayTag& InputTag, const FGameplayTag& AbilityTag, FGameplayEventData& OutPayload)
{
	OutPayload = FGameplayEventData();
	return true;
}
