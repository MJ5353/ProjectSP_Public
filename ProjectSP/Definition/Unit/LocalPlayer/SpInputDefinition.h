#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SpInputDefinition.generated.h"

class UInputAction;

// ==================================================

USTRUCT(Blueprintable)
struct FSpInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<const UInputAction> InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	FGameplayTag InputTag;
};

// ==================================================

UCLASS()
class PROJECTSP_API USpInputDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<FSpInputAction> NativeInputActions; // locomotion같은 것들

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<FSpInputAction> AbilityInputActions;
	
	const UInputAction* FindNativeInputActionForTag(const FGameplayTag& InputTag) const;
};
