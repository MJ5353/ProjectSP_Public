#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "SpAttributeSet.h"
#include "SpSpeedAttributeSet.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpSpeedAttributeSet : public USpAttributeSet
{
	GENERATED_BODY()
	
public:
	SP_ATTRIBUTE_ACCESSORS(USpSpeedAttributeSet, MoveSpeed);
	SP_ATTRIBUTE_ACCESSORS(USpSpeedAttributeSet, AttackSpeed);
	SP_ATTRIBUTE_ACCESSORS(USpSpeedAttributeSet, RotationRateDegreesPerSecond);
	SP_ATTRIBUTE_ACCESSORS(USpSpeedAttributeSet, RotationToleranceDegrees);
	
protected:
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MoveSpeed, Category = "MJ - Replicate")
	FGameplayAttributeData MoveSpeed;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackSpeed, Category = "MJ - Replicate")
	FGameplayAttributeData AttackSpeed;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RotationRateDegreesPerSecond, Category = "MJ - Replicate")
	FGameplayAttributeData RotationRateDegreesPerSecond;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_RotationToleranceDegrees, Category = "MJ - Replicate")
	FGameplayAttributeData RotationToleranceDegrees;
	
public:
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldAttribute);
	
	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldAttribute);

	UFUNCTION()
	void OnRep_RotationRateDegreesPerSecond(const FGameplayAttributeData& OldAttribute);

	UFUNCTION()
	void OnRep_RotationToleranceDegrees(const FGameplayAttributeData& OldAttribute);

private:
	void ApplyMoveSpeedToMovement(float NewMoveSpeed) const;
	void ApplyRotationRateToMovement(float RotPerSecond) const;
};
