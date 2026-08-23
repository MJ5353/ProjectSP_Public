#pragma once

#include "CoreMinimal.h"
#include "SpGrantedAttributeSets.generated.h"

class USpAbilitySystemComponent;
class USpAttributeSet;

// ==================================================

USTRUCT()
struct FSpGrantedAttributeSets
{
	GENERATED_BODY()

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<USpAttributeSet>> AttributeSets;

public:
	void AddAttributeSet(USpAttributeSet* AttributeSet);
	void TakeFromAbilitySystem(USpAbilitySystemComponent* ASC);
};
