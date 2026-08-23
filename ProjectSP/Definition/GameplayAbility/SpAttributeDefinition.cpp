#include "SpAttributeDefinition.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/Ability/SpGrantedAttributeSets.h"
#include "ProjectSP/Attribute/SpAttributeSet.h"

// ==================================================

void USpAttributeDefinition::GiveToAbilitySystem(USpAbilitySystemComponent* ASC, FSpGrantedAttributeSets* OutGrantedAttributeSets) const
{
	check(ASC);

	if (!ASC->IsOwnerActorAuthoritative())
		return;

	for (TSubclassOf<UAttributeSet> AttributeSetClass : AttributeSets)
	{
		if (!AttributeSetClass)
			continue;

		USpAttributeSet* NewSet = NewObject<USpAttributeSet>(ASC->GetOwner(), AttributeSetClass);
		ASC->AddAttributeSetSubobject(NewSet);

		if (OutGrantedAttributeSets)
			OutGrantedAttributeSets->AddAttributeSet(NewSet);
	}
	
	for (const FSpAttributeInitValue& InitialValue : AttributeInitialValues)
	{
		if (!InitialValue.Attribute.IsValid())
			continue;

		const TSubclassOf<UAttributeSet> AttributeSetClass = InitialValue.Attribute.GetAttributeSetClass();
		if (!ASC->GetAttributeSet(AttributeSetClass))
			continue;

		ASC->SetNumericAttributeBase(InitialValue.Attribute, InitialValue.Value);
	}
}
