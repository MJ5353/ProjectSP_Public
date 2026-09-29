#include "SpGrantedAbilityHandles.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"

// ==================================================

void FSpGrantedAbilityHandles::AddAbilitySpecHandle(const FGameplayTag& Tag, const FGameplayAbilitySpecHandle& Handle)
{
	if (!Tag.IsValid() || !Handle.IsValid())
		return;

	if (TagToAbilitySpecHandle.Contains(Tag))
	{
		ensureMsgf(false, TEXT("Duplicate ability tag registered: %s"), *Tag.ToString());
		return;
	}

	TagToAbilitySpecHandle.Add(Tag, Handle);
}

void FSpGrantedAbilityHandles::TakeFromAbilitySystem(USpAbilitySystemComponent* ASC)
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
		return;

	for (const TPair<FGameplayTag, FGameplayAbilitySpecHandle>& Pair : TagToAbilitySpecHandle)
	{
		if (Pair.Value.IsValid())
			ASC->ClearAbility(Pair.Value);
	}

	TagToAbilitySpecHandle.Reset();
}

bool FSpGrantedAbilityHandles::HasSpecHandle(const FGameplayTag& Tag) const
{
	return Tag.IsValid() ? TagToAbilitySpecHandle.Contains(Tag) : false;
}
