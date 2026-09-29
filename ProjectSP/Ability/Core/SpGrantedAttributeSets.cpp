#include "SpGrantedAttributeSets.h"

#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Attribute/SpAttributeSet.h"

// ==================================================

void FSpGrantedAttributeSets::AddAttributeSet(USpAttributeSet* AttributeSet)
{
	if (IsValid(AttributeSet))
		AttributeSets.Add(AttributeSet);
}

void FSpGrantedAttributeSets::TakeFromAbilitySystem(USpAbilitySystemComponent* ASC)
{
	if (!ASC || !ASC->IsOwnerActorAuthoritative())
		return;

	for (UAttributeSet* Set : AttributeSets)
	{
		if (IsValid(Set))
			ASC->RemoveSpawnedAttribute(Set);
	}

	AttributeSets.Reset();
}
