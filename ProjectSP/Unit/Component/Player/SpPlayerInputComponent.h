#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "SpPlayerInputComponent.generated.h"

class UInputComponent;

// ==================================================

DECLARE_MULTICAST_DELEGATE_ThreeParams(FSpTaggedInputEvent, FGameplayTag, const FInputActionValue&, ETriggerEvent);

// ------------------------------------------------

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpPlayerInputComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	FSpTaggedInputEvent OnTaggedInput;

private:
	bool bIsInputInitialized = false;
	
public:
	void SetUp(UInputComponent* InputComponent);

	// input action
	void InputStarted(const FInputActionValue& Value, FGameplayTag InputTag);
	void InputTriggered(const FInputActionValue& Value, FGameplayTag InputTag);
	void InputCompleted(const FInputActionValue& Value, FGameplayTag InputTag);
	void InputCanceled(const FInputActionValue& Value, FGameplayTag InputTag);
	
	// get
	bool IsGameplayInputEnabled_Client() const;
};
