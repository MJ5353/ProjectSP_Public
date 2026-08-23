#include "SpAttackAttributeSet.h"
#include "Net/UnrealNetwork.h"

// ==================================================

void USpAttackAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetAttackPowerAttribute())
		NewValue = FMath::Max(NewValue, 0.0f);
}

void USpAttackAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetAttackPowerAttribute())
		NewValue = FMath::Max(NewValue, 0.0f);
}

void USpAttackAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(USpAttackAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
}

void USpAttackAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpAttackAttributeSet, AttackPower, OldAttackPower);
}
