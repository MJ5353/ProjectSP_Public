#pragma once

#include "CoreMinimal.h"
#include "SpAIUnitDefinition.h"
#include "SpEnemyAIDefinition.generated.h"

struct FSpAIAbilityOption;

// ==================================================

UCLASS()
class PROJECTSP_API USpEnemyAIDefinition : public USpAIDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TArray<FSpAIAbilityOption> AbilityOptions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float HomeRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float ChaseRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", UIMin = "0.01", UIMax = "1.0"))
	float PatrolSpeed = 0.5f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAggroDefinition> AggroDefinition;
	
	// ------------------------------------------------

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void SortAbilityOptionsByPriority();
};
