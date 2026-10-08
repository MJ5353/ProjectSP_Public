#include "SpGameplayTags.h"

// ==================================================

namespace SpGameplayTags
{
	// ability key
	UE_DEFINE_GAMEPLAY_TAG(AbilityKeyTag, "GameplayAbility.Key");
	
	// ability target
	UE_DEFINE_GAMEPLAY_TAG(AbilityTargetTag_None, "GameplayAbility.Target.None");
	UE_DEFINE_GAMEPLAY_TAG(AbilityTargetTag_Self, "GameplayAbility.Target.Self");
	UE_DEFINE_GAMEPLAY_TAG(AbilityTargetTag_Hostile, "GameplayAbility.Target.Hostile");
	UE_DEFINE_GAMEPLAY_TAG(AbilityTargetTag_Location, "GameplayAbility.Target.Location");

	// ability condition
	UE_DEFINE_GAMEPLAY_TAG(AbilityConditionTag_TargetFacing, "GameplayAbility.Condition.TargetFacing");

	// ability behavior
	UE_DEFINE_GAMEPLAY_TAG(AbilityBehaviorTag_Persist, "GameplayAbility.Behavior.Persist");
	
	// ability modifier scope
	UE_DEFINE_GAMEPLAY_TAG(AbilityModifierScopeTag_OncePerActivation, "GameplayAbility.Modifier.Scope.OncePerActivation");
	UE_DEFINE_GAMEPLAY_TAG(AbilityModifierScopeTag_OncePerTargetPerActivation, "GameplayAbility.Modifier.Scope.OncePerTargetPerActivation");
	UE_DEFINE_GAMEPLAY_TAG(AbilityModifierScopeTag_PerHit, "GameplayAbility.Modifier.Scope.PerHit");

	// effect behavior
	UE_DEFINE_GAMEPLAY_TAG(EffectBehaviorTag_Persist, "GameplayEffect.Behavior.Persist");
	UE_DEFINE_GAMEPLAY_TAG(EffectBehaviorTag_NoInstigator, "GameplayEffect.Behavior.NoInstigator");

	// unit flag
	UE_DEFINE_GAMEPLAY_TAG(UnitFlagTag_Targetable, "Unit.BattleFlag.Targetable");
	
	// unit state
	UE_DEFINE_GAMEPLAY_TAG(UnitStateTag, "Unit.State");
	UE_DEFINE_GAMEPLAY_TAG(UnitStateTag_Select, "Unit.State.Select");
	UE_DEFINE_GAMEPLAY_TAG(UnitStateTag_Dead, "Unit.State.Dead");
	UE_DEFINE_GAMEPLAY_TAG(UnitStateTag_Respawn, "Unit.State.Respawn");

	// unit reaction
	UE_DEFINE_GAMEPLAY_TAG(UnitReactionTag_Hit, "Unit.Reaction.Hit");
}
