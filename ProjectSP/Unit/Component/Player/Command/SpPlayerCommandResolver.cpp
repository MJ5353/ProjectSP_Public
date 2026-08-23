#include "SpPlayerCommandResolver.h"
#include "NavigationSystem.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

FSpPlayerCommandResolution FSpPlayerCommandResolver::Resolve_Client(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay) const
{
	FSpPlayerCommandResolution Resolution;

	FHitResult HitResult;
	ASpUnit* TargetUnit = TraceCursorRay(SourceUnit, CursorRay, HitResult) ? Cast<ASpUnit>(HitResult.GetActor()) : nullptr;
	if (TargetUnit && TargetUnit->IsAttackable(&SourceUnit))
	{
		Resolution.Kind = ESpPlayerCommandKind::PrimaryAttack;
		Resolution.TargetUnit = TargetUnit;
	}

	return Resolution;
}

bool FSpPlayerCommandResolver::Resolve_Server(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay, FSpPlayerCommandResolution& OutResolution) const
{
	OutResolution = FSpPlayerCommandResolution();

	FHitResult HitResult;
	if (!TraceCursorRay(SourceUnit, CursorRay, HitResult))
		return false;

	ASpUnit* TargetUnit = Cast<ASpUnit>(HitResult.GetActor());
	if (TargetUnit && TargetUnit->IsAttackable(&SourceUnit))
	{
		OutResolution.Kind = ESpPlayerCommandKind::PrimaryAttack;
		OutResolution.TargetUnit = TargetUnit;
		return true;
	}

	OutResolution.Kind = ESpPlayerCommandKind::Move;
	return TryProjectMoveDestination_Server(SourceUnit, HitResult, OutResolution.MoveDestination);
}

bool FSpPlayerCommandResolver::ResolveMoveDestination_Server(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay, FVector& OutMoveDestination) const
{
	FHitResult HitResult;
	return TraceCursorRay(SourceUnit, CursorRay, HitResult) && TryProjectMoveDestination_Server(SourceUnit, HitResult, OutMoveDestination);
}

bool FSpPlayerCommandResolver::TraceCursorRay(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay, FHitResult& OutHitResult) const
{
	UWorld* World = SourceUnit.GetWorld();
	if (!World || CursorRay.Origin.ContainsNaN() || CursorRay.Direction.ContainsNaN())
		return false;

	const FVector Direction = CursorRay.Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
		return false;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SpPlayerCommandTrace), true, &SourceUnit);
	QueryParams.AddIgnoredActor(&SourceUnit);
	const FVector TraceEnd = CursorRay.Origin + Direction * Settings.CursorTraceDistance;
	return World->LineTraceSingleByChannel(OutHitResult, CursorRay.Origin, TraceEnd, Settings.TraceChannel, QueryParams);
}

bool FSpPlayerCommandResolver::TryProjectMoveDestination_Server(const ASpUnit& SourceUnit, const FHitResult& HitResult, FVector& OutMoveDestination) const
{
	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(SourceUnit.GetWorld());
	if (!NavigationSystem)
		return false;

	FNavLocation NavLocation;
	if (!NavigationSystem->ProjectPointToNavigation(HitResult.ImpactPoint, NavLocation, Settings.NavigationProjectionExtent))
		return false;

	OutMoveDestination = NavLocation.Location;
	return true;
}
