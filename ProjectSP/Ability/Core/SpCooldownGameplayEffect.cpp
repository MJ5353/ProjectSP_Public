#include "SpCooldownGameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "NativeGameplayTags.h"

// ==================================================

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SpCooldownDuration, "SetByCaller.SpCooldownDuration");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SpCooldownActive, "GameplayEffect.Cooldown.Active");

// ------------------------------------------------

USpCooldownGameplayEffect::USpCooldownGameplayEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	FSetByCallerFloat DurationSetByCaller;
	DurationSetByCaller.DataTag = GetDurationSetByCallerTag();
	DurationMagnitude = FGameplayEffectModifierMagnitude(DurationSetByCaller);

	UTargetTagsGameplayEffectComponent* TargetTagsComponent = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("CooldownTargetTags"));
	GEComponents.Add(TargetTagsComponent);

	FInheritedTagContainer GrantedTags;
	GrantedTags.AddTag(TAG_SpCooldownActive);
	TargetTagsComponent->SetAndApplyTargetTagChanges(GrantedTags);
}

FGameplayTag USpCooldownGameplayEffect::GetDurationSetByCallerTag()
{
	return TAG_SpCooldownDuration;
}
