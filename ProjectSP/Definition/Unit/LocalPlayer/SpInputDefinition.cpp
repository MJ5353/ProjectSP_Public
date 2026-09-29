#include "SpInputDefinition.h"

// ==================================================

const UInputAction* USpInputDefinition::FindInputActionForTag(const FGameplayTag& InputTag) const
{
	const FSpInputActionBinding* Binding = FindInputBindingForTag(InputTag);
	return Binding ? Binding->InputAction.Get() : nullptr;
}

const FSpInputActionBinding* USpInputDefinition::FindInputBindingForTag(const FGameplayTag& InputTag) const
{
	for (const FSpInputActionBinding& Binding : InputActions)
	{
		if (Binding.InputAction && Binding.InputTag == InputTag)
			return &Binding;
	}

	return nullptr;
}
