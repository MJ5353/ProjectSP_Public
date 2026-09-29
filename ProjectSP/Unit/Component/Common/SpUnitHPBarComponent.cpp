#include "SpUnitHPBarComponent.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Component/SpUnitClientGatewayComponent.h"
#include "ProjectSP/UI/SpAttributeWidget.h"
#include "ProjectSP/UI/SpUserWidget.h"

// ==================================================

bool USpUnitHPBarComponent::IsClientPresentationRequired() const
{
	return GetWidgetClass() != nullptr;
}

void USpUnitHPBarComponent::PrepareClientPresentation(const FSpUnitData& UnitData)
{
	ASpUnit* Unit = GetOwner<ASpUnit>();
	if (!Unit)
		return;

	OwnerUnit = Unit;
	PresentationUnitUid = UnitData.UnitUid;
	
	TryReportPresentationReady();
}

void USpUnitHPBarComponent::StopClientPresentation()
{
	if (USpUserWidget* SpWidget = Cast<USpUserWidget>(GetUserWidgetObject()))
		SpWidget->SetOwnerUnit(nullptr);

	OwnerUnit.Reset();
	PresentationUnitUid = 0;
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

void USpUnitHPBarComponent::RefreshPresentation_Client()
{
	TryReportPresentationReady();
}

void USpUnitHPBarComponent::BeginPlay()
{
	Super::BeginPlay();

	TryReportPresentationReady();
}

void USpUnitHPBarComponent::InitWidget()
{
	Super::InitWidget();

	TryReportPresentationReady();
}

bool USpUnitHPBarComponent::TryApplyOwnerUnit()
{
	if (!OwnerUnit.IsValid())
		return false;

	UUserWidget* UserWidget = GetUserWidgetObject();
	if (!UserWidget)
		return false;

	USpUserWidget* SpWidget = Cast<USpUserWidget>(UserWidget);
	if (!SpWidget)
		return false;

	if (USpAttributeWidget* AttributeWidget = Cast<USpAttributeWidget>(SpWidget))
		return AttributeWidget->PrepareAttributePresentation(OwnerUnit.Get());

	SpWidget->SetOwnerUnit(OwnerUnit.Get());
	return true;
}

void USpUnitHPBarComponent::TryReportPresentationReady()
{
	if (PresentationUnitUid == 0 || !TryApplyOwnerUnit())
		return;

	if (USpUnitClientGatewayComponent* ClientGateway = OwnerUnit->GetClientGateway())
		ClientGateway->ReportPresentationReady_Client(this, PresentationUnitUid);
}
