#pragma once

#include "NativeGameplayTags.h"

// ==================================================

namespace SpGameplayTags
{
	// ability key
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityKeyTag);
	
	// ability target
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityTargetTag_None);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityTargetTag_Self);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityTargetTag_Hostile);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityTargetTag_Location);

	// ability condition
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityConditionTag_TargetFacing);

	// ability behavior
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityBehaviorTag_Persist);

	// ability modifier scope
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityModifierScopeTag_OncePerActivation);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityModifierScopeTag_OncePerTargetPerActivation);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(AbilityModifierScopeTag_PerHit);

	// effect behavior
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EffectBehaviorTag_Persist);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(EffectBehaviorTag_NoInstigator);

	// unit flag
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnitFlagTag_Targetable);

	// unit state
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnitStateTag);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnitStateTag_Select);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnitStateTag_Dead);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnitStateTag_Respawn);
	
	// unit reaction
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(UnitReactionTag_Hit);
}
