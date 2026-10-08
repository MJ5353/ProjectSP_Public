#include "SpAttributeWidget.h"
#include "ProjectSP/Attribute/SpHPAttributeSet.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"

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

bool USpAttributeWidget::PrepareAttributePresentation(ASpUnit* InOwnerUnit)
{
	SetOwnerUnit(InOwnerUnit);
	
	// 소유자 설정, ASC 구독이 완료됐는지
	return IsAttributePresentationReady(InOwnerUnit);
}

// set

void USpAttributeWidget::SetOwnerUnit(ASpUnit* InOwnerUnit)
{
	Super::SetOwnerUnit(InOwnerUnit);
	
	ClearAttributeSubscribe();
	TrySetAttributeSubscribe();
}

void USpAttributeWidget::NativeDestruct()
{
	ClearAttributeSubscribe();

	Super::NativeDestruct();
}

// attribute

bool USpAttributeWidget::TrySetAttributeSubscribe()
{
	if (!OwnerUnit.IsValid() || Attributes.IsEmpty())
		return false;

	USpAbilitySystemComponent* ASC = OwnerUnit->GetSpAbilitySystemComponent();
	if (!ASC)
		return false;

	BoundASC = ASC;

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (!Attribute.IsValid())
		{
			ClearAttributeSubscribe();
			return false;
		}

		const bool bAlreadySubscribed = AttributeSubscribeData.ContainsByPredicate
		(
			[&Attribute](const FSpAttributeSubscribeData& Data)
			{
				return Data.Attribute == Attribute;
			}
		);

		if (bAlreadySubscribed)
		{
			ClearAttributeSubscribe();
			return false;
		}

		FDelegateHandle Handle = ASC->RegisterAttributeChangeCallback(Attribute, this, &USpAttributeWidget::OnAttributeChange);
		if (!Handle.IsValid())
		{
			ClearAttributeSubscribe();
			return false;
		}

		AttributeSubscribeData.Emplace(Attribute, Handle);
	}

	RefreshAttributeValues();
	return true;
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
	if (!HasValidMaxHp())
		return;

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
	if (!HasValidMaxHp())
		return;

	const FGameplayAttribute MaxHPAttribute = USpHPAttributeSet::GetMaxHPAttribute();
	if (ChangeData.Attribute == MaxHPAttribute)
	{
		RefreshAttributeValues();
		return;
	}

	K2_OnAttributeValueChanged(ChangeData.Attribute, ChangeData.OldValue, ChangeData.NewValue);
}

// get

bool USpAttributeWidget::IsAttributePresentationReady(const ASpUnit* InOwnerUnit) const
{
	if (!InOwnerUnit || OwnerUnit.Get() != InOwnerUnit || Attributes.IsEmpty())
		return false;

	USpAbilitySystemComponent* ASC = InOwnerUnit->GetSpAbilitySystemComponent();
	if (!ASC || BoundASC.Get() != ASC || AttributeSubscribeData.Num() != Attributes.Num())
		return false;

	for (const FGameplayAttribute& Attribute : Attributes)
	{
		if (!Attribute.IsValid())
			return false;

		const FSpAttributeSubscribeData* SubscribeData = AttributeSubscribeData.FindByPredicate
		(
			[&Attribute](const FSpAttributeSubscribeData& Data)
			{
				return Data.Attribute == Attribute;
			}
		);
		if (!SubscribeData || !SubscribeData->Handle.IsValid())
			return false;
	}

	return true;
}

bool USpAttributeWidget::HasValidMaxHp() const
{
	const FGameplayAttribute MaxHPAttribute = USpHPAttributeSet::GetMaxHPAttribute();
	return !Attributes.Contains(MaxHPAttribute)
		|| GetAttributeValueByAttribute(MaxHPAttribute) > KINDA_SMALL_NUMBER;
}
