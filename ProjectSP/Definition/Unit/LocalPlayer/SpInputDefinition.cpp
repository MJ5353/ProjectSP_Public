#include "SpInputDefinition.h"

// ==================================================

const UInputAction* USpInputDefinition::FindNativeInputActionForTag(const FGameplayTag& InputTag) const
{
	for (const FSpInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
			return Action.InputAction;
	}

	// [mj] todo) 여기서 크래시 내고싶은데...
	return nullptr;
}
