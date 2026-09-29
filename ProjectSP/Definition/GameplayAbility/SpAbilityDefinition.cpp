#include "SpAbilityDefinition.h"
#include "GameplayAbilitySpecHandle.h"
#include "ProjectSP/Ability/Core/SpGrantedAbilityHandles.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Ability/Core/SpGameplayAbility.h"

// ==================================================

void USpAbilityDefinition::GiveToAbilitySystem(USpAbilitySystemComponent* ASC, FSpGrantedAbilityHandles* OutGrantedHandles, UObject* SourceObject) const
{
	check(ASC);
	
	if (!OutGrantedHandles || !ASC->IsOwnerActorAuthoritative())
		return;

	for (const auto Pair: AbilityMap)
	{
		TSubclassOf<USpGameplayAbility> AbilityClass = Pair.Value;
		FGameplayTag AbilityTag = Pair.Key;
		
		if (!IsValid(AbilityClass) || !AbilityTag.IsValid())
			continue;
		
		// EGameplayAbilityInstancingPolicy를 참고하면 어떤 형태로 Ability를 가질 수 있는지 확인할 수 있다.
		// 개중에 NonInstanced는 CDO를 뜻하며 비용이 제일 낮다. 
		// 다만, 언제나 CDO만 쓸 수 있는것은 아님. 실시간 변경되는 값들의 경우 Instanced를 사용해야 할수도
		USpGameplayAbility* AbilityCDO = AbilityClass->GetDefaultObject<USpGameplayAbility>();

		// Ability를 사용하기 전에 Spec으로 묶어야 함. GiveAbility의 매개변수가 Spec
		FGameplayAbilitySpec AbilitySpec(AbilityCDO, 1);
		AbilitySpec.SourceObject = SourceObject;
		AbilitySpec.GetDynamicSpecSourceTags().AddTag(AbilityTag);
		
		// 중복 방지
		if (OutGrantedHandles->HasSpecHandle(AbilityTag))
			continue;
		
		const FGameplayAbilitySpecHandle AbilitySpecHandle = ASC->GiveAbility(AbilitySpec);
		OutGrantedHandles->AddAbilitySpecHandle(AbilityTag, AbilitySpecHandle);
	}
}
