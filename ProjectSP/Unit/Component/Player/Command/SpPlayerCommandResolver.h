#pragma once

#include "CoreMinimal.h"
#include "SpPlayerCommandTypes.h"

class ASpUnit;

// ==================================================

struct FSpPlayerCommandResolverSettings
{
	float CursorTraceDistance = 20000.0f;
	FVector NavigationProjectionExtent = FVector(100.0f, 100.0f, 300.0f);
	ECollisionChannel TraceChannel = ECC_GameTraceChannel1;
};

// CursorRay를 명령 의도로 해석한다. Client는 예측용, Server는 권위 있는 결과를 만든다.
class FSpPlayerCommandResolver
{
	FSpPlayerCommandResolverSettings Settings;

public:
	void Configure(FSpPlayerCommandResolverSettings InSettings) { Settings = InSettings; }

	FSpPlayerCommandResolution Resolve_Client(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay) const;
	bool Resolve_Server(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay, FSpPlayerCommandResolution& OutResolution) const;
	bool ResolveMoveDestination_Server(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay, FVector& OutMoveDestination) const;

private:
	bool TraceCursorRay(const ASpUnit& SourceUnit, const FSpPlayerCommandCursorRay& CursorRay, FHitResult& OutHitResult) const;
	bool TryProjectMoveDestination_Server(const ASpUnit& SourceUnit, const FHitResult& HitResult, FVector& OutMoveDestination) const;
};
