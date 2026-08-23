#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputDefinition.h"
#include "SpInputComponent.generated.h"

class USpInputDefinition;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	template <class UserClass, typename FuncType>
	void BindNativeAction(const USpInputDefinition* InputDefinition, const FGameplayTag& InputTag, ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func);
	
	template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
	void BindAbilityActions(const USpInputDefinition* InputDefinition, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles);
};

// template ------------------------------------------------

template <class UserClass, typename FuncType>
void USpInputComponent::BindNativeAction(const USpInputDefinition* InputDefinition, const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent, UserClass* Object, FuncType Func)
{
	check(InputDefinition);

	// InputDefinition은 활성화 가능한 InputAction을 담고 있다.
	if (const UInputAction* IA = InputDefinition->FindNativeInputActionForTag(InputTag))
	{
		BindAction(IA, TriggerEvent, Object, Func);
	}
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void USpInputComponent::BindAbilityActions(const USpInputDefinition* InputDefinition, UserClass* Object, PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc, TArray<uint32>& BindHandles)
{
	check(InputDefinition);

	// AbilityAction에 대해서는 그냥 모든 InputAction에 다 바인딩
	for (const FSpInputAction& Action : InputDefinition->AbilityInputActions)
	{
		if (Action.InputAction && Action.InputTag.IsValid())
		{
			if (PressedFunc)
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered, Object, PressedFunc, Action.InputTag).GetHandle());

			if (ReleasedFunc)
				BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed, Object, ReleasedFunc, Action.InputTag).GetHandle());
		}
	}
}
