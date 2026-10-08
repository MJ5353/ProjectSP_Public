#include "SpUnitStateComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

void USpUnitStateComponent::OnInitUnit()
{
}

void USpUnitStateComponent::OnClearUnit()
{
}

// dead

void USpUnitStateComponent::RegisterDeadEvent()
{
	if (DeadEventHandle.IsValid())
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return;

	USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent();
	if (!ASC)
		return;

	const FGameplayTag DeadTag = SpGameplayTags::UnitStateTag_Dead;
	
	FGameplayEventMulticastDelegate& Delegate = ASC->GenericGameplayEventCallbacks.FindOrAdd(DeadTag);
	DeadEventHandle = Delegate.AddUObject(this, &ThisClass::HandleGameplayEvent_Dead);
}

void USpUnitStateComponent::UnregisterDeadEvent()
{
	if (!DeadEventHandle.IsValid())
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
	{
		DeadEventHandle.Reset();
		return;
	}

	if (USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent())
	{
		const FGameplayTag DeadTag = SpGameplayTags::UnitStateTag_Dead;
	
		if (FGameplayEventMulticastDelegate* Delegate = ASC->GenericGameplayEventCallbacks.Find(DeadTag))
			Delegate->Remove(DeadEventHandle);
	}

	DeadEventHandle.Reset();
}

void USpUnitStateComponent::HandleGameplayEvent_Dead(const FGameplayEventData* Payload)
{
	if (IsOnDead)
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return;

	IsOnDead = true;
	Unit->Dead();
}
