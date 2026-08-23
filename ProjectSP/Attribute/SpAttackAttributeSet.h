#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SpAttributeSet.h"
#include "SpAttackAttributeSet.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpAttackAttributeSet : public USpAttributeSet
{
	GENERATED_BODY()

public:
	SP_ATTRIBUTE_ACCESSORS(USpAttackAttributeSet, AttackPower);

protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPower, Category = "MJ - Replicate")
	FGameplayAttributeData AttackPower;

public:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower);
};
