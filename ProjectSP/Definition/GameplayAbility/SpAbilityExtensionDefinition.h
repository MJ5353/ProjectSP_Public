#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SpAbilityExtensionDefinition.generated.h"

// ==================================================

USTRUCT(BlueprintType)
struct FSpAbilityExtensionGroup
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	TArray<FGameplayTag> Extensions;
};

// ==================================================

UCLASS()
class PROJECTSP_API USpAbilityExtensionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Ability|Extension")
	TMap<FGameplayTag, FSpAbilityExtensionGroup> Options;

private:
	mutable TMap<FGameplayTag, FGameplayTag> ExtensionToAbility;
	mutable bool bExtensionIndexBuilt = false;
	
public:
	// Find ------------------------------------------------
	
	FGameplayTag FindAbilityKeyByExtension(FGameplayTag ExtensionTag) const;
	bool FindOption(FGameplayTag AbilityKey, FGameplayTag ExtensionTag) const;
	const FSpAbilityExtensionGroup* FindOptionByExtension(FGameplayTag ExtensionTag) const;

	// Post ------------------------------------------------
	
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditUndo() override;
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
	// Build Data ------------------------------------------
	
	bool BuildExtensionIndex(TMap<FGameplayTag, FGameplayTag>& OutIndex, TArray<FText>& OutErrors) const;
	void RebuildExtensionIndex() const;
};
