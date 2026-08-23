#include "SpUnitHPBarComponent.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/UI/SpUserWidget.h"

// ==================================================

bool USpUnitHPBarComponent::PrepareClientPresentation(const FSpUnitData& UnitData)
{
	ASpUnit* Unit = GetOwner<ASpUnit>();
	if (!Unit)
		return false;
	
	OwnerUnit = Unit;
	TryApplyOwnerUnit();
	
	return true;
}

void USpUnitHPBarComponent::OnUnitActive(bool bActive)
{
	SetComponentTickEnabled(bActive);
	
	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!UserWidget)
		return;
	
	if (bActive)
		UserWidget->SetVisibility(ESlateVisibility::Visible);
	else
		UserWidget->SetVisibility(ESlateVisibility::Hidden);
}

void USpUnitHPBarComponent::BeginPlay()
{
	Super::BeginPlay();

	TryApplyOwnerUnit();
}

void USpUnitHPBarComponent::InitWidget()
{
	Super::InitWidget();

	TryApplyOwnerUnit();
}

void USpUnitHPBarComponent::TryApplyOwnerUnit()
{
	if (!OwnerUnit.IsValid())
		return;
	
	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!UserWidget)
		return;

	USpUserWidget* SpWidget = Cast<USpUserWidget>(UserWidget);
	if (!SpWidget)
		return;
	
	SpWidget->SetOwnerUnit(OwnerUnit.Get());
}


