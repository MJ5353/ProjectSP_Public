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
	
	if (Attribute == GetRotationRateDegreesPerSecondAttribute())
		ApplyRotationRateToMovement(NewValue);
}

void USpSpeedAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
		ApplyMoveSpeedToMovement(NewValue);
}

void USpSpeedAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetRotationRateDegreesPerSecondAttribute())
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
	ApplyMoveSpeedToMovement(GetMoveSpeed());
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

void USpSpeedAttributeSet::ApplyMoveSpeedToMovement(const float NewMoveSpeed) const
{
	UAbilitySystemComponent* AbilitySystemComponent = GetOwningAbilitySystemComponent();
	ASpUnit* Unit = AbilitySystemComponent ? Cast<ASpUnit>(AbilitySystemComponent->GetAvatarActor()) : nullptr;
	if (!Unit)
		return;

	if (UCharacterMovementComponent* MovementComponent = Unit->GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = NewMoveSpeed;
	}
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
