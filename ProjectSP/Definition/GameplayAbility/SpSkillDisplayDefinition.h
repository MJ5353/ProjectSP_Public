#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SpSkillDisplayDefinition.generated.h"

class UTexture2D;

// ==================================================

USTRUCT(BlueprintType)
struct FSpSkillDisplayInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Skill|Display")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Skill|Display", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Skill|Display")
	TObjectPtr<UTexture2D> Icon;
};

// ------------------------------------------------

USTRUCT(BlueprintType)
struct FSpOrderedSkillDisplay
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Skill|Display")
	FGameplayTag AbilityKey;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Skill|Display")
	TArray<FGameplayTag> ExtensionOrder;
};

// ------------------------------------------------

UCLASS(BlueprintType)
class PROJECTSP_API USpSkillDisplayDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	TArray<FSpOrderedSkillDisplay> SkillOrder;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	TMap<FGameplayTag, FSpSkillDisplayInfo> DisplayInfoByTag;

	UFUNCTION(BlueprintPure, Category="MJ - Skill")
	bool TryGetDisplayInfo(FGameplayTag Tag, FSpSkillDisplayInfo& OutInfo) const;
};
