#include "SpSpeedAttributeSet.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

void USpSpeedAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);
	
	if (Attribute == GetMoveSpeedAttribute())
	{
		UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
		AActor* Actor = AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor() : nullptr;
		if (!Actor)
			return;
		
		ASpUnit* Unit = Cast<ASpUnit>(Actor);
		if (!Unit)
			return;
		
		if (UCharacterMovementComponent* MovementComponent = Unit->GetCharacterMovement())
			MovementComponent->MaxWalkSpeed = NewValue;
	}
	else if (Attribute == GetRotationRateDegreesPerSecondAttribute())
	{
		ApplyRotationRateToMovement(NewValue);
	}
}

void USpSpeedAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetMoveSpeedAttribute())
	{
		UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
		AActor* Actor = AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor() : nullptr;
		if (!Actor)
			return;
		
		ASpUnit* Unit = Cast<ASpUnit>(Actor);
		if (!Unit)
			return;
		
		if (UCharacterMovementComponent* MovementComponent = Unit->GetCharacterMovement())
			MovementComponent->MaxWalkSpeed = GetMoveSpeed();
	}
	else if (ModifiedAttribute == GetAttackSpeedAttribute())
	{
		// [mj] todo) anim에 반영
	}
	else if (ModifiedAttribute == GetRotationRateDegreesPerSecondAttribute())
	{
		ApplyRotationRateToMovement(GetRotationRateDegreesPerSecond());
	}
}

void USpSpeedAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(USpSpeedAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USpSpeedAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USpSpeedAttributeSet, RotationRateDegreesPerSecond, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USpSpeedAttributeSet, RotationToleranceDegrees, COND_None, REPNOTIFY_Always);
}

void USpSpeedAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpSpeedAttributeSet, MoveSpeed, OldAttribute);
}

void USpSpeedAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpSpeedAttributeSet, AttackSpeed, OldAttribute);
}

void USpSpeedAttributeSet::OnRep_RotationRateDegreesPerSecond(const FGameplayAttributeData& OldAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpSpeedAttributeSet, RotationRateDegreesPerSecond, OldAttribute);
	ApplyRotationRateToMovement(GetRotationRateDegreesPerSecond());
}

void USpSpeedAttributeSet::OnRep_RotationToleranceDegrees(const FGameplayAttributeData& OldAttribute)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpSpeedAttributeSet, RotationToleranceDegrees, OldAttribute);
}

void USpSpeedAttributeSet::ApplyRotationRateToMovement(const float RotPerSecond) const
{
	UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
	ASpUnit* Unit = AbilitySystemComponent ? Cast<ASpUnit>(AbilitySystemComponent->GetAvatarActor()) : nullptr;
	if (!Unit)
		return;

	if (UCharacterMovementComponent* MovementComponent = Unit->GetCharacterMovement())
		MovementComponent->RotationRate.Yaw = FMath::Max(RotPerSecond, 0.0f);
}
