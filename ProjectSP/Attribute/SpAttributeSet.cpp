#include "SpAttributeSet.h"

DEFINE_LOG_CATEGORY_STATIC(LogSpAttribute, Log, All);

// ==================================================

void LogAttributeChange(const USpAttributeSet* AttributeSet, const TCHAR* ValueType, const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
#if !UE_BUILD_SHIPPING
	const AActor* OwnerActor = AttributeSet->GetOwningActor();
	const TCHAR* NetContext = OwnerActor && OwnerActor->HasAuthority() ? TEXT("Authority") : TEXT("Client");

	UE_LOG(LogSpAttribute, Log, TEXT("[%s][%s][%s] %s: %.2f -> %.2f"), *GetNameSafe(OwnerActor), NetContext, ValueType, *Attribute.GetName(), OldValue, NewValue);
#endif
}

void USpAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
	LogAttributeChange(this, TEXT("Current"), Attribute, OldValue, NewValue);
}

void USpAttributeSet::PostAttributeBaseChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) const
{
	Super::PostAttributeBaseChange(Attribute, OldValue, NewValue);
	LogAttributeChange(this, TEXT("Base"), Attribute, OldValue, NewValue);
}
