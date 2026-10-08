#include "SpPlayerCameraComponent.h"
#include "Curves/CurveVector.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpCameraDefinition.h"
#include "ProjectSP/Unit/Define/SpUnitData.h"

// ==================================================

void USpPlayerCameraComponent::PrepareClientPresentation(const FSpUnitData& UnitData)
{
	if (GetNetMode() == NM_DedicatedServer || !UnitData.UnitDefinition)
		return;
	
	CameraDefinition = UnitData.UnitDefinition->CameraDefinition;
}

void USpPlayerCameraComponent::StopClientPresentation()
{
	CameraDefinition = nullptr;
}

void USpPlayerCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
{
	if (!CameraDefinition)
	{
		Super::GetCameraView(DeltaTime, DesiredView);
		return;
	}

	FVector Location = GetPivotLocation();
	FRotator Rotation = GetPivotRotation();
	Rotation.Pitch = FMath::ClampAngle(Rotation.Pitch, CameraDefinition->ViewPitchMin, CameraDefinition->ViewPitchMax);
	
	// curve vector에 맞게 pivot 처리
	if (CameraDefinition->TargetOffsetCurve)
	{
		const FVector TargetOffset = CameraDefinition->TargetOffsetCurve->GetVectorValue(Rotation.Pitch);
		Location = Location + Rotation.RotateVector(TargetOffset);
	}
	
	if (APawn* TargetPawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = TargetPawn->GetController<APlayerController>())
			PC->SetControlRotation(Rotation);
	}
	
	SetWorldLocationAndRotation(Location, Rotation);

	DesiredView.Location = Location;
	DesiredView.Rotation = Rotation;
	DesiredView.FOV = FieldOfView;
	DesiredView.OrthoWidth = OrthoWidth;
	DesiredView.OrthoNearClipPlane = OrthoNearClipPlane;
	DesiredView.OrthoFarClipPlane = OrthoFarClipPlane;
	DesiredView.AspectRatio = AspectRatio;
	DesiredView.bConstrainAspectRatio = bConstrainAspectRatio;
	DesiredView.bUseFieldOfViewForLOD = bUseFieldOfViewForLOD;
	DesiredView.ProjectionMode = ProjectionMode;
	DesiredView.PostProcessBlendWeight = PostProcessBlendWeight;
	
	if (PostProcessBlendWeight > 0.0f)
		DesiredView.PostProcessSettings = PostProcessSettings;
}

FVector USpPlayerCameraComponent::GetPivotLocation() const
{
	const AActor* TargetActor = GetOwner();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		// BaseEyeHeight를 고려하여, ViewLocation을 반환
		return TargetPawn->GetPawnViewLocation();
	}

	return TargetActor->GetActorLocation();
}

FRotator USpPlayerCameraComponent::GetPivotRotation() const
{
	const AActor* TargetActor = GetOwner();
	check(TargetActor);

	if (const APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		// 보통 Pawn의 ControlRotation을 반환
		return TargetPawn->GetViewRotation();
	}

	return TargetActor->GetActorRotation();
}


