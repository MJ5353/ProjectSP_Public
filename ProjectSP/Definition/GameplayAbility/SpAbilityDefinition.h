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

UCLASS()
class PROJECTSP_API USpAbilityDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	TMap<FGameplayTag, TSubclassOf<USpGameplayAbility>> AbilityMap;
	
	// ------------------------------------------------
	
	void GiveToAbilitySystem(USpAbilitySystemComponent* ASC, FSpGrantedAbilityHandles* OutGrantedHandles, UObject* SourceObject = nullptr) const;
};
