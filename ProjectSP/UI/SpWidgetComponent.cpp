#include "SpWidgetComponent.h"
#include "CommonUIUtils.h"
#include "SpUserWidget.h"

// ==================================================

void USpWidgetComponent::SetOwner(ASpUnit* InOwnerUnit)
{
	UUserWidget* UserWidget = GetWidget();
	if (!ensure(UserWidget))
		return;
	
	USpUserWidget* SpWidget = Cast<USpUserWidget>(UserWidget);
	if (!ensure(SpWidget))
		return;
	
	SpWidget->SetOwnerUnit(InOwnerUnit);
}

