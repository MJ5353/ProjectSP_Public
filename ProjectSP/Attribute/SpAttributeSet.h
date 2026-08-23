#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "SpAttributeSet.generated.h"

// ==================================================

/**
 * Gameplay Attribute 접근 함수 생성
 *
 * GetHealthAttribute()
 * GetHealth()
 * SetHealth()
 * InitHealth()
 */
#define SP_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTSP_API USpAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const override;
};
