#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "ProjectSP/Definition/GameplayAbility/SpAttributeDefinition.h"
#include "ProjectSP/Definition/GameplayAbility/SpAbilityDefinition.h"
#include "SpUnitDefinition.generated.h"

class USpAIDefinition;
class UBehaviorTree;
class ASpUnit;
class USpCameraDefinition;
class USpAbilityDefinition;
class USpAbilityExtensionDefinition;
class USpSkillDisplayDefinition;
class USpInputDefinition;
struct FSpAttributeInitValue;
struct FSpInputMappingContext;

// ==================================================

UCLASS()
class PROJECTSP_API USpUnitDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	FString Name;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ASpUnit> UnitClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpCameraDefinition> CameraDefinition;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpInputDefinition> InputDefinition;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<FSpInputMappingContext> InputMappingContexts;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TArray<TObjectPtr<USpAbilityDefinition>> AbilityDefinitions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting")
	TObjectPtr<USpSkillDisplayDefinition> SkillDisplayDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting")
	TObjectPtr<USpAbilityExtensionDefinition> AbilityExtensionDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting")
	TArray<FGameplayTag> InitiallyEquippedExtensions;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpAttributeDefinition> AttributeDefinition;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpAIDefinition> AIDefinition;
	
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting|Death", meta=(ClampMin="0.0", UIMin="0.0"))
	float DeadProcessTime = 0.0f;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
