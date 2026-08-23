#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "CommonUserWidget.h"
#include "SpUserWidget.h"
#include "SpAttributeWidget.generated.h"

class ASpUnit;
class USpAbilitySystemComponent;
struct FOnAttributeChangeData;

// ==================================================

struct FSpAttributeSubscribeData
{
	FGameplayAttribute Attribute;
	FDelegateHandle Handle;

	FSpAttributeSubscribeData() = default;
	FSpAttributeSubscribeData(const FGameplayAttribute& InAttribute, const FDelegateHandle& InHandle)
		: Attribute(InAttribute), Handle(InHandle)
	{
	}
};

// ==================================================

UCLASS()
class PROJECTSP_API USpAttributeWidget : public USpUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="MJ - Attribute")
	float GetAttributeValue() const;

	UFUNCTION(BlueprintCallable, Category="MJ - Attribute")
	float GetAttributeValueByIndex(int32 AttributeIndex) const;

	UFUNCTION(BlueprintCallable, Category="MJ - Attribute")
	float GetAttributeValueByAttribute(FGameplayAttribute InAttribute) const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="MJ - Attribute")
	void K2_OnAttributeValueChanged(FGameplayAttribute ChangedAttribute, float OldValue, float NewValue);
	
	virtual void SetOwnerUnit(ASpUnit* InOwnerUnit) override;
	
	virtual void NativeDestruct() override;
	virtual void SetAttributeSubscribe();
	virtual void ClearAttributeSubscribe();

	virtual void RefreshAttributeValues();
	virtual void OnAttributeChange(const FOnAttributeChangeData& ChangeData);

private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting", meta=(AllowPrivateAccess="true"))
	TArray<FGameplayAttribute> Attributes;

	TWeakObjectPtr<USpAbilitySystemComponent> BoundASC;
	TArray<FSpAttributeSubscribeData> AttributeSubscribeData;
};

