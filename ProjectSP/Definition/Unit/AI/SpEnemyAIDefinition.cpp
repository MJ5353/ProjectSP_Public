#pragma once

#include "SpEnemyAIDefinition.h"
#include "ProjectSP/Unit/AI/FSpAIAbilityOption.h"

// ==================================================

#if WITH_EDITOR
void USpEnemyAIDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName ChangedPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	if (ChangedPropertyName == GET_MEMBER_NAME_CHECKED(USpEnemyAIDefinition, AbilityOptions))
		SortAbilityOptionsByPriority();
}
#endif

void USpEnemyAIDefinition::SortAbilityOptionsByPriority()
{
	AbilityOptions.StableSort([](const FSpAIAbilityOption& LHS, const FSpAIAbilityOption& RHS)
	{
		return LHS.Priority > RHS.Priority;
	});
}

