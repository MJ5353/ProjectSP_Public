#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Animation/AnimInstance.h"
#include "SpAnimInstance.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Transient, VisibleAnywhere, BlueprintReadWrite, Category = "MJ - Runtime")
	float GroundDistance = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	FGameplayTagBlueprintPropertyMap GameplayTagPropertyMap; // gameplay tag와 anim Instance의 속성값을 매핑해주는 struct

	// ------------------------------------------------
	
	// animation이 최초로 초기화됐을 때 호출되는 함수
	virtual void NativeInitializeAnimation() override;
	
	void InitializeWithAbilitySystem(UAbilitySystemComponent* ASC);
};
