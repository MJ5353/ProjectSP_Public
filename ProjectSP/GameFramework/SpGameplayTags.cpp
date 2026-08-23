#include "SpGameplayTags.h"
#include "GameplayTagsManager.h"

// ==================================================

// static 구현!
FSpGameplayTags FSpGameplayTags::GameplayTags;

FSpGameplayTags::FSpGameplayTags()
{
}

FSpGameplayTags::~FSpGameplayTags()
{
}

void FSpGameplayTags::InitializeNativeTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	GameplayTags.AddAllTags(Manager);
}

void FSpGameplayTags::AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment)
{
	FString Comment = FString(TEXT("(Native) ")) + FString(TagComment);
	OutTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName(TagName), Comment);
}

void FSpGameplayTags::AddAllTags(UGameplayTagsManager& Manager)
{
	// ability
	AddTag(AbilitySlotTag_BasicAttack, "GameplayAbility.AbilitySlot.BasicAttack", "");
	AddTag(AbilitySlotTag_Secondary, "GameplayAbility.AbilitySlot.Secondary", "");
	AddTag(AbilityConditionTag_NeedTarget, "GameplayAbility.Condition.NeedTarget", "");
	
	// state
	AddTag(StateTag, "Unit.State", "");
	AddTag(StateTag_Select, "Unit.State.Select", "");
	AddTag(StateTag_Dead, "Unit.State.Dead", "");
	AddTag(StateTag_Respawn, "Unit.State.Respawn", "");

	// battle flag
	AddTag(BattleFlagTag_Targetable, "Unit.BattleFlag.Targetable", "");

	// input
	AddTag(InputTag_PrimaryClick, "Input.Click.Primary", "");
	AddTag(InputTag_PrimaryClick_Reserve, "Input.Click.Primary.Reserve", "");
	AddTag(InputTag_SecondaryClick, "Input.Click.Secondary", "");
	AddTag(InputTag_Move_WASD, "Input.Move.WASD", "");
	AddTag(InputTag_Look_Mouse, "Input.Look.Mouse", "");
}
