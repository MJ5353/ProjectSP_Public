#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "ProjectSP/Unit/Define/SpUnitData.h"
#include "ProjectSP/Unit/Interface/SpUnitClientListener.h"
#include "SpPlayerCameraComponent.generated.h"

class USpCameraDefinition;

// ==================================================

UCLASS()
class PROJECTSP_API USpPlayerCameraComponent : public UCameraComponent, public ISpUnitClientListener
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TObjectPtr<USpCameraDefinition> CameraDefinition;
	
	virtual void PrepareClientPresentation(const FSpUnitData& UnitData) override;
	virtual void StopClientPresentation() override;

protected:
	virtual void GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView) override;
	FVector GetPivotLocation() const;
	FRotator GetPivotRotation() const;
};

