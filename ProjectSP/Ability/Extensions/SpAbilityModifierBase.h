#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "SpAbilityModifierBase.generated.h"

class USpGameplayAbility;

// ==================================================

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTSP_API USpAbilityModifierBase : public UObject
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	FGameplayTag ModifierTag;

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="MJ - Ability Modifier")
	void OnActivateAbility(USpGameplayAbility* Ability);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="MJ - Ability Modifier")
	void OnCommitAbility(USpGameplayAbility* Ability);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="MJ - Ability Modifier")
	void OnEndAbility(USpGameplayAbility* Ability);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="MJ - Ability Modifier")
	void OnCancelAbility(USpGameplayAbility* Ability);
	
	// Implementation
	virtual void OnActivateAbility_Implementation(USpGameplayAbility* Ability);
	virtual void OnCommitAbility_Implementation(USpGameplayAbility* Ability);
	virtual void OnEndAbility_Implementation(USpGameplayAbility* Ability);
	virtual void OnCancelAbility_Implementation(USpGameplayAbility* Ability);
};
