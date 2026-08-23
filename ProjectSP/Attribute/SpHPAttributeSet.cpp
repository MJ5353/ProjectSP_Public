#include "SpHPAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"

// ==================================================

USpHPAttributeSet::USpHPAttributeSet()
{
}

void USpHPAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetCurrentHPAttribute())
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHP());
}

void USpHPAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	const FGameplayAttribute& ModifiedAttribute = Data.EvaluatedData.Attribute;
	if (ModifiedAttribute == GetDamageAttribute())
	{
		const float AppliedDamage = FMath::Max(GetDamage(), 0.0f);

		// 다음 대미지에 누적되지 않도록 즉시 초기화
		SetDamage(0.0f);

		if (AppliedDamage > 0.0f)
		{
			float NewHP = FMath::Clamp(GetCurrentHP() - AppliedDamage, 0.0f, GetMaxHP());
			SetAndCheckCurrentHp(NewHP);

			if (USpAbilitySystemComponent* SpASC = Cast<USpAbilitySystemComponent>(GetOwningAbilitySystemComponent()))
				SpASC->NotifyDamageApplied(Data.EffectSpec.GetEffectContext().GetOriginalInstigator(), AppliedDamage);
		}
	}
	else if (ModifiedAttribute == GetCurrentHPAttribute())
	{
		float NewHP = FMath::Clamp(GetCurrentHP(), 0.0f, GetMaxHP());
		SetAndCheckCurrentHp(NewHP);
	}
	else if (ModifiedAttribute == GetMaxHPAttribute())
	{
		float NewMaxHP = FMath::Max(GetMaxHP(),1.0f);
		SetMaxHP(NewMaxHP);

		float NewHP = FMath::Clamp(GetCurrentHP(),0.0f,GetMaxHP());
		SetAndCheckCurrentHp(NewHP);
	}
}

void USpHPAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(USpHPAttributeSet, CurrentHP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USpHPAttributeSet, MaxHP, COND_None, REPNOTIFY_Always);
}

void USpHPAttributeSet::SetAndCheckCurrentHp(float NewHp)
{
	SetCurrentHP(NewHp);
	
	if (NewHp <= 0.f)
		OnDead();
}

void USpHPAttributeSet::OnDead()
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC)
		return;

	FGameplayTag Tag = FSpGameplayTags::Get().StateTag_Dead;
	ASC->SetLooseGameplayTagCount(Tag, 1, EGameplayTagReplicationState::TagOnly);
}

void USpHPAttributeSet::OnRep_CurrentHP(const FGameplayAttributeData& OldHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpHPAttributeSet, CurrentHP, OldHP);
}

void USpHPAttributeSet::OnRep_MaxHP(const FGameplayAttributeData& OldMaxHP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USpHPAttributeSet, MaxHP, OldMaxHP);
}


