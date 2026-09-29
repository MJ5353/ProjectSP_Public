#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ProjectSP/Input/SpInputBehavior.h"
#include "SpInputDefinition.generated.h"

class UInputAction;

// ==================================================

USTRUCT(Blueprintable)
struct FSpInputActionBinding
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<const UInputAction> InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	FGameplayTag InputTag;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	FGameplayTag AbilityTag;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TSubclassOf<USpInputBehavior> BehaviorClass;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	bool bClearStateOnNewInput = false;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	bool bSendTriggeredUpdates = false;
};

// ==================================================

UCLASS()
class PROJECTSP_API USpInputDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<FSpInputActionBinding> InputActions;

	// ------------------------------------------------
	
	const UInputAction* FindInputActionForTag(const FGameplayTag& InputTag) const;
	const FSpInputActionBinding* FindInputBindingForTag(const FGameplayTag& InputTag) const;
};
