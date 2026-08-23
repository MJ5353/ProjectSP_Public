#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "SpAttributeDefinition.generated.h"

class USpAbilitySystemComponent;
class USpAttributeSet;
struct FSpGrantedAttributeSets;
struct FSpAttributeInitValue;

// ==================================================

USTRUCT(BlueprintType)
struct FSpAttributeInitValue
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	FGameplayAttribute Attribute;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	float Value = 0.0f;
};

// ==================================================

UCLASS()
class PROJECTSP_API USpAttributeDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<FSpAttributeInitValue> AttributeInitialValues;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<TSubclassOf<USpAttributeSet>> AttributeSets;
	
public:
	void GiveToAbilitySystem(USpAbilitySystemComponent* ASC, FSpGrantedAttributeSets* OutGrantedAttributeSets) const;
};
