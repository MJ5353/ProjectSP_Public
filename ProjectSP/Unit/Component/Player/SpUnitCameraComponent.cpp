#include "SpUnitCameraComponent.h"
#include "Curves/CurveVector.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpCameraDefinition.h"
#include "ProjectSP/Unit/Define/SpUnitData.h"

// ==================================================

bool USpUnitCameraComponent::PrepareClientPresentation(const FSpUnitData& UnitData)
{
	// 정의 데이터가 아직 없으면 Gateway가 액터를 숨긴 채 다음 복제 갱신을 기다린다.
	if (GetNetMode() == NM_DedicatedServer || !UnitData.UnitDefinition)
		return false;
	
	CameraDefinition = UnitData.UnitDefinition->CameraDefinition;
	return CameraDefinition != nullptr;
}

void USpUnitCameraComponent::StopClientPresentation()
{
	CameraDefinition = nullptr;
}

void USpUnitCameraComponent::GetCameraView(float DeltaTime, FMinimalViewInfo& DesiredView)
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

	// [mj] todo) pitch
	
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

FVector USpUnitCameraComponent::GetPivotLocation() const
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

FRotator USpUnitCameraComponent::GetPivotRotation() const
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


