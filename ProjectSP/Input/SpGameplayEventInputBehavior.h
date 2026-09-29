#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectSP/Input/SpInputBehavior.h"
#include "SpGameplayEventInputBehavior.generated.h"

// ==================================================

UCLASS(Blueprintable)
class PROJECTSP_API USpGameplayEventInputBehavior : public USpInputBehavior
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	FGameplayTag EventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	ETriggerEvent SendOnTriggerEvent = ETriggerEvent::Started;

public:
	virtual ESpInputBehaviorRequest HandleInput(ASpPlayerUnit& Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FSpInputActionBinding& Binding, bool bIsActiveBehavior) override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category="MJ - Gameplay Event", meta=(DisplayName="Build Gameplay Event Payload"))
	bool BuildEventPayload(ASpPlayerUnit* Unit, const FInputActionValue& Value, const FGameplayTag& InputTag, const FGameplayTag& AbilityTag, FGameplayEventData& OutPayload);

	virtual bool BuildEventPayload_Implementation(ASpPlayerUnit* Unit, const FInputActionValue& Value, const FGameplayTag& InputTag, const FGameplayTag& AbilityTag, FGameplayEventData& OutPayload);
};
