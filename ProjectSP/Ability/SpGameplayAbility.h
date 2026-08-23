#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SpGameplayAbility.generated.h"

// ==================================================

UENUM(BlueprintType)
enum class EMjAbilityActivationPolicy : uint8
{
	// input이 trigger된 경우
	OnInputTriggered,
	
	// input이 held된 경우
	WhileInputActive,
	
	// avatar가 생성된 경우 (패시브라고 보면 됨)
	OnSpawn,
};

// ==================================================

UCLASS()
class PROJECTSP_API USpGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	EMjAbilityActivationPolicy ActivationPolicy;
	
	// ------------------------------------------------
	
	USpGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};