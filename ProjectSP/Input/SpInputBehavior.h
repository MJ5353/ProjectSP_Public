#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "UObject/Object.h"
#include "SpInputBehavior.generated.h"

class ASpPlayerUnit;
class USpPlayerActionComponent;
struct FSpInputActionBinding;

// ==================================================

UENUM(BlueprintType)
enum class ESpInputBehaviorRequest : uint8
{
	None,
	Set,
	Clear
};

// ------------------------------------------------

UCLASS(Abstract, Blueprintable)
class PROJECTSP_API USpInputBehavior : public UObject
{
	GENERATED_BODY()

public:
	virtual ESpInputBehaviorRequest HandleInput(ASpPlayerUnit& Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FSpInputActionBinding& Binding, bool bIsActiveBehavior);

	virtual void TickBehavior(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, FGameplayTag SourceInputTag, float DeltaTime) {}
	virtual void ClearBehavior(ASpPlayerUnit* Unit) {}

protected:
	UFUNCTION(BlueprintNativeEvent, Category="MJ - Input", meta=(DisplayName="Handle Input"))
	ESpInputBehaviorRequest K2_HandleInput(ASpPlayerUnit* Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FGameplayTag& InputTag, const FGameplayTag& AbilityTag, bool bIsActiveBehavior);
	
	virtual ESpInputBehaviorRequest K2_HandleInput_Implementation(ASpPlayerUnit* Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FGameplayTag& InputTag, const FGameplayTag& AbilityTag, bool bIsActiveBehavior);
};
