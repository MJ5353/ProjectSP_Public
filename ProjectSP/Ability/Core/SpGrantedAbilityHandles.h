#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "SpGrantedAbilityHandles.generated.h"

class USpAbilitySystemComponent;

// ==================================================

USTRUCT()
struct FSpGrantedAbilityHandles
{
	GENERATED_BODY()

private:
	// 서버가 지급한 Ability를 Definition 교체 시 회수하기 위한 기록이다.
	UPROPERTY(Transient)
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> TagToAbilitySpecHandle;

public:
	void AddAbilitySpecHandle(const FGameplayTag& Tag, const FGameplayAbilitySpecHandle& Handle);
	void TakeFromAbilitySystem(USpAbilitySystemComponent* ASC);
	
	bool HasSpecHandle(const FGameplayTag& Tag) const;
};
