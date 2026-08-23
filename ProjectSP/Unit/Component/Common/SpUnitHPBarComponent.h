#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "ProjectSP/Unit/Interface/SpUnitClientListener.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpUnitHPBarComponent.generated.h"

class ASpUnit;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitHPBarComponent : public UWidgetComponent, public ISpUnitManageListener, public ISpUnitClientListener
{
	GENERATED_BODY()
	
	UPROPERTY(Transient, VisibleAnywhere, Category="MJ - Runtime")
	TWeakObjectPtr<ASpUnit> OwnerUnit;
	
public:
	virtual bool PrepareClientPresentation(const FSpUnitData& UnitData) override;
	virtual void OnUnitActive(bool bActive) override;

protected:
	virtual void BeginPlay() override;
	virtual void InitWidget() override;

private:
	void TryApplyOwnerUnit();
};

