#include "SpUnitDefinition.h"
#include "ProjectSP/Definition/GameplayAbility/SpAbilityExtensionDefinition.h"
#include "ProjectSP/Definition/GameplayAbility/SpSkillDisplayDefinition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult USpUnitDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (!SkillDisplayDefinition)
		return ParentResult;

	bool bInvalid = false;
	TSet<FGameplayTag> SeenSkills;
	TSet<FGameplayTag> SeenExtensions;
	for (const FSpOrderedSkillDisplay& Skill : SkillDisplayDefinition->SkillOrder)
	{
		const FGameplayTag AbilityKey = Skill.AbilityKey;
		if (!AbilityKey.IsValid() || SeenSkills.Contains(AbilityKey))
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Invalid or duplicate displayed skill tag: %s"), *AbilityKey.ToString())));
			bInvalid = true;
			continue;
		}

		SeenSkills.Add(AbilityKey);
		if (!SkillDisplayDefinition->DisplayInfoByTag.Contains(AbilityKey))
		{
			Context.AddError(FText::FromString(FString::Printf(TEXT("Missing display info for skill: %s"), *AbilityKey.ToString())));
			bInvalid = true;
		}

		for (const FGameplayTag& ExtensionTag : Skill.ExtensionOrder)
		{
			if (!ExtensionTag.IsValid() || SeenExtensions.Contains(ExtensionTag))
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Invalid or duplicate displayed extension tag: %s"), *ExtensionTag.ToString())));
				bInvalid = true;
				continue;
			}

			SeenExtensions.Add(ExtensionTag);
			if (!AbilityExtensionDefinition || !AbilityExtensionDefinition->FindOption(AbilityKey, ExtensionTag))
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Extension %s does not belong to displayed skill %s."), *ExtensionTag.ToString(), *AbilityKey.ToString())));
				bInvalid = true;
			}

			if (!SkillDisplayDefinition->DisplayInfoByTag.Contains(ExtensionTag))
			{
				Context.AddError(FText::FromString(FString::Printf(TEXT("Missing display info for extension: %s"), *ExtensionTag.ToString())));
				bInvalid = true;
			}
		}
	}

	return bInvalid || ParentResult == EDataValidationResult::Invalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif
