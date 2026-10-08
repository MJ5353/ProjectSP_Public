#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SpAttributeSet.h"
#include "SpHPAttributeSet.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpHPAttributeSet : public USpAttributeSet
{
	GENERATED_BODY()
	
public:
	SP_ATTRIBUTE_ACCESSORS(USpHPAttributeSet, CurrentHP);
	SP_ATTRIBUTE_ACCESSORS(USpHPAttributeSet, MaxHP);
	SP_ATTRIBUTE_ACCESSORS(USpHPAttributeSet, Damage);

	// ------------------------------------------------

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentHP, Category = "MJ - Replicate")
	FGameplayAttributeData CurrentHP;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHP, Category = "MJ - Replicate")
	FGameplayAttributeData MaxHP;
	
	UPROPERTY(BlueprintReadOnly, Category = "MJ - Replicate")
	FGameplayAttributeData Damage;

	// ------------------------------------------------
	
	USpHPAttributeSet();
	
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void SetAndCheckCurrentHp(float NewHp);
	void OnDead();
	
	UFUNCTION()
	void OnRep_CurrentHP(const FGameplayAttributeData& OldHP);

	UFUNCTION()
	void OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP);
};
