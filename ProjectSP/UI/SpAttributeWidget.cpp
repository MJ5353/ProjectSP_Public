#include "SpAttributeWidget.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"

// ==================================================

float USpAttributeWidget::GetAttributeValue() const
{
	return GetAttributeValueByIndex(0);
}

float USpAttributeWidget::GetAttributeValueByIndex(int32 AttributeIndex) const
{
	if (!Attributes.IsValidIndex(AttributeIndex))
		return 0.f;

	return GetAttributeValueByAttribute(Attributes[AttributeIndex]);
}

float USpAttributeWidget::GetAttributeValueByAttribute(FGameplayAttribute InAttribute) const
{
	if (!BoundASC.IsValid() || !InAttribute.IsValid())
		return 0.f;

	return BoundASC->GetNumericAttribute(InAttribute);
}

void USpAttributeWidget::SetOwnerUnit(ASpUnit* InOwnerUnit)
{
	Super::SetOwnerUnit(InOwnerUnit);
	
	ClearAttributeSubscribe();
	SetAttributeSubscribe();
}

void USpAttributeWidget::NativeDestruct()
{
	ClearAttributeSubscribe();

	Super::NativeDestruct();
}

void USpAttributeWidget::SetAttributeSubscribe()
{
	if (!OwnerUnit.IsValid() || Attributes.IsEmpty())
		return;

	USpAbilitySystemComponent* ASC = OwnerUnit->GetSpAbilitySystemComponent();
	if (!ASC)
		return;

	BoundASC = ASC;

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (!Attribute.IsValid())
			continue;

		const bool bAlreadySubscribed = AttributeSubscribeData.ContainsByPredicate(
			[&Attribute](const FSpAttributeSubscribeData& Data)
			{
				return Data.Attribute == Attribute;
			}
		);

		if (bAlreadySubscribed)
			continue;

		FDelegateHandle Handle = ASC->RegisterAttributeChangeCallback(Attribute, this, &USpAttributeWidget::OnAttributeChange);
		if (Handle.IsValid())
			AttributeSubscribeData.Emplace(Attribute, Handle);
	}

	RefreshAttributeValues();
}

void USpAttributeWidget::ClearAttributeSubscribe()
{
	if (BoundASC.IsValid())
	{
		for (FSpAttributeSubscribeData& Data : AttributeSubscribeData)
			BoundASC->UnregisterAttributeChangeCallback(Data.Attribute, Data.Handle);
	}

	BoundASC.Reset();
	AttributeSubscribeData.Reset();
}

void USpAttributeWidget::RefreshAttributeValues()
{
	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (!Attribute.IsValid())
			continue;

		const float CurrentValue = GetAttributeValueByAttribute(Attribute);
		K2_OnAttributeValueChanged(Attribute, CurrentValue, CurrentValue);
	}
}

void USpAttributeWidget::OnAttributeChange(const FOnAttributeChangeData& ChangeData)
{
	K2_OnAttributeValueChanged(ChangeData.Attribute, ChangeData.OldValue, ChangeData.NewValue);
}


