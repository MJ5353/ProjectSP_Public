#pragma once

#include "Containers/UnrealString.h"
#include "Containers/Map.h"
#include "GameplayTagContainer.h"

class UGameplayTagsManager;

// ==================================================

// C++에서 Tag를 쓰기 위함
struct FSpGameplayTags
{
private:
	// static 변수 초기화는 .cpp에 해주는 것을 잊지 말기!
	// singleton으로 사용하겠다는 의미
	static FSpGameplayTags GameplayTags;
	
	// 싱글톤으로 사용할 클래스를 만들 때 생성자와 소멸자를 private으로 지정하고 복사생성자를 차단
	FSpGameplayTags();
	~FSpGameplayTags();

public:
	// ability slot
	FGameplayTag AbilitySlotTag_BasicAttack;
	FGameplayTag AbilitySlotTag_Secondary;
	
	// ability condition
	FGameplayTag AbilityConditionTag_NeedTarget;

	// state
	FGameplayTag StateTag;
	FGameplayTag StateTag_Select;
	FGameplayTag StateTag_Dead;
	FGameplayTag StateTag_Respawn;
	
	// battle flag
	FGameplayTag BattleFlagTag_Targetable;
	
	// input
	FGameplayTag InputTag_PrimaryClick;
	FGameplayTag InputTag_PrimaryClick_Reserve;
	FGameplayTag InputTag_SecondaryClick;
	FGameplayTag InputTag_Move_WASD;
	FGameplayTag InputTag_Look_Mouse;
	
	// ------------------------------------------------
	
	static const FSpGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeTags();

	void AddTag(FGameplayTag& OutTag, const ANSICHAR* TagName, const ANSICHAR* TagComment);
	void AddAllTags(UGameplayTagsManager& Manager);
};