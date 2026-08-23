#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SpAbilityDefinition.generated.h"

struct FSpGrantedAbilityHandles;
class USpAttributeSet;
class USpAbilitySystemComponent;
class USpGameplayAbility;

// ==================================================

USTRUCT(BlueprintType)
struct FSpAbilityTagPairData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USpGameplayAbility> Ability = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag Tag;
};

// ==================================================

UCLASS()
class PROJECTSP_API USpAbilityDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<FSpAbilityTagPairData> AbilityTagPairs;
	
	// ------------------------------------------------
	
public:
	void GiveToAbilitySystem(USpAbilitySystemComponent* ASC, FSpGrantedAbilityHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;
};
