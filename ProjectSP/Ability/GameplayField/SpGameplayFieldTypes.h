#pragma once

#include "CoreMinimal.h"
#include "SpGameplayFieldTypes.generated.h"

// ==================================================

UENUM(BlueprintType)
enum class ESpGameplayFieldApplicationPolicy : uint8
{
	Once,
	OnEnter,
	Periodic,
};

UENUM(BlueprintType)
enum class ESpGameplayFieldBindingPolicy : uint8
{
	World,
	FollowSource,
};

// ==================================================

USTRUCT(BlueprintType)
struct FSpGameplayFieldConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field")
	ESpGameplayFieldBindingPolicy BindingPolicy = ESpGameplayFieldBindingPolicy::World;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field")
	ESpGameplayFieldApplicationPolicy ApplicationPolicy = ESpGameplayFieldApplicationPolicy::OnEnter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field", meta=(ClampMin="0.01"))
	float Duration = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field", meta=(ClampMin="0.01"))
	float EvaluationPeriod = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field")
	bool bEvaluateImmediately = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field")
	bool bRemoveAppliedEffectsOnExit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MJ - Field")
	bool bRemoveAppliedEffectsOnFinish = false;
};
