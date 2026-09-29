#include "SpSkillDisplayDefinition.h"

// ==================================================

bool USpSkillDisplayDefinition::TryGetDisplayInfo(const FGameplayTag Tag, FSpSkillDisplayInfo& OutInfo) const
{
	const FSpSkillDisplayInfo* Info = DisplayInfoByTag.Find(Tag);
	if (!Info)
		return false;

	OutInfo = *Info;
	return true;
}
