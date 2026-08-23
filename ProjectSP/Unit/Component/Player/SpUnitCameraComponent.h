#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "ProjectSP/Unit/Define/SpUnitData.h"
#include "ProjectSP/Unit/Interface/SpUnitClientListener.h"
#include "SpUnitCameraComponent.generated.h"

class USpCameraDefinition;

// ==================================================

UCLASS()
class PROJECTSP_API USpUnitCameraComponent : public UCameraComponent, public ISpUnitClientListener
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TObjectPtr<USpCameraDefinition> CameraDefinition;
	
	virtual bool PrepareClientPresentation(const FSpUnitData& UnitData) override;
	virtual void StopClientPresentation() override;

protected:
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
	FVector GetPivotLocation() const;
	FRotator GetPivotRotation() const;
};

