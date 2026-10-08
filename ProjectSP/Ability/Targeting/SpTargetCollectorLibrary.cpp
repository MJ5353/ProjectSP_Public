#include "SpTargetCollectorLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GenericTeamAgentInterface.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Subsystem/UnitRegistrySubsystem.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

namespace
{
	struct FCollectedTarget
	{
		ASpUnit* Target = nullptr;
		float DistanceSquared = 0.0f;
		float SegmentHitTime = 0.0f;
	};

	FVector GetDirection2D(const FVector& Direction)
	{
		const FVector Direction2D(Direction.X, Direction.Y, 0.0f);
		return Direction2D.IsNearlyZero() ? FVector::ForwardVector : Direction2D.GetSafeNormal();
	}

	float GetCollisionRadius(const ASpUnit& Unit)
	{
		const UCapsuleComponent* Capsule = Unit.GetCapsuleComponent();
		return Capsule ? FMath::Max(Capsule->GetScaledCapsuleRadius(), 0.0f) : 0.0f;
	}

	bool TryGetSegmentCircleHitTime(const FVector& SegmentStart, const FVector& SegmentEnd, const FVector& CircleCenter, const float Radius, float& OutHitTime)
	{
		OutHitTime = 0.0f;

		const FVector2D Start(SegmentStart.X, SegmentStart.Y);
		const FVector2D End(SegmentEnd.X, SegmentEnd.Y);
		const FVector2D Center(CircleCenter.X, CircleCenter.Y);
		const FVector2D Direction = End - Start;
		const FVector2D FromCenter = Start - Center;

		const float A = FVector2D::DotProduct(Direction, Direction);
		const float RadiusSquared = FMath::Square(FMath::Max(Radius, 0.0f));

		if (A <= UE_KINDA_SMALL_NUMBER)
			return FromCenter.SquaredLength() <= RadiusSquared;

		if (FromCenter.SquaredLength() <= RadiusSquared)
			return true;

		const float B = 2.0f * FVector2D::DotProduct(FromCenter, Direction);
		const float C = FVector2D::DotProduct(FromCenter, FromCenter) - RadiusSquared;
		const float Discriminant = B * B - 4.0f * A * C;

		if (Discriminant < 0.0f)
			return false;

		const float EnterTime = (-B - FMath::Sqrt(Discriminant)) / (2.0f * A);
		if (EnterTime < 0.0f || EnterTime > 1.0f)
			return false;

		OutHitTime = EnterTime;
		return true;
	}

	bool MatchesRelation(const ASpUnit* Source, uint32 SourceUnitUid, FGenericTeamId SourceTeamId, const ASpUnit& Candidate, const FSpTargetQuery& Query)
	{
		bool bIsSelf = Source ? Source == &Candidate : Candidate.GetUnitUid() == SourceUnitUid;
		if (bIsSelf)
		{
			bool bFriendly = Query.Relation == ESpTargetRelation::Friendly || Query.Relation == ESpTargetRelation::Any;
			return Query.Relation == ESpTargetRelation::Self || (Query.bIncludeSource && bFriendly);
		}
		
		if (Query.Relation == ESpTargetRelation::Self)
			return false;

		ETeamAttitude::Type Attitude = ETeamAttitude::Neutral;
		if (!Source)
		{
			const uint8 SourceId = SourceTeamId.GetId();
			const uint8 CandidateId = Candidate.GetGenericTeamId().GetId();
			const uint8 NoTeamId = FGenericTeamId::NoTeam.GetId();

			if (SourceId != NoTeamId && CandidateId != NoTeamId)
				Attitude = SourceId == CandidateId ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
		}
		else
		{
			Attitude = Source->GetTeamAttitudeTowards(Candidate);
		}
		
		switch (Query.Relation)
		{
			case ESpTargetRelation::Hostile:
				return Attitude == ETeamAttitude::Hostile;

			case ESpTargetRelation::Friendly:
				return Attitude == ETeamAttitude::Friendly;

			case ESpTargetRelation::Neutral:
				return Attitude == ETeamAttitude::Neutral;

			case ESpTargetRelation::Any:
				return true;

			default:
				return false;
		}
	}

	bool IsValidCandidate(const ASpUnit* Source, uint32 SourceUnitUid, FGenericTeamId SourceTeamId, const ASpUnit& Candidate, const FSpTargetQuery& Query)
	{
		if (Candidate.CheckDead() || !MatchesRelation(Source, SourceUnitUid, SourceTeamId, Candidate, Query))
			return false;

		return !Query.bRequireTargetable || Candidate.HasTag(SpGameplayTags::UnitFlagTag_Targetable, false);
	}

	bool GetSearchBounds(const FSpTargetQuery& Query, FVector& OutCenter, float& OutRadius)
	{
		OutCenter = Query.Origin;
		OutRadius = 0.0f;

		const FVector Direction = GetDirection2D(Query.Direction);
		switch (Query.Shape)
		{
			case ESpTargetShape::Circle:
			case ESpTargetShape::Sector:
				OutRadius = Query.Radius;
				return true;

			case ESpTargetShape::Line:
			{
				const float HalfLength = Query.Length * 0.5f;
				OutCenter = Query.Origin + Direction * HalfLength;
				OutRadius = HalfLength + Query.Radius;
				return true;
			}

			case ESpTargetShape::Box:
			{
				const float HalfLength = Query.Length * 0.5f;
				const float HalfWidth = Query.Width * 0.5f;
				OutCenter = Query.Origin + Direction * HalfLength;
				OutRadius = FMath::Sqrt(HalfLength * HalfLength + HalfWidth * HalfWidth);
				return true;
			}

			case ESpTargetShape::Segment:
			{
				const float HalfLength = FVector::Dist2D(Query.Origin, Query.SegmentEnd) * 0.5f;
				OutCenter = (Query.Origin + Query.SegmentEnd) * 0.5f;
				OutRadius = HalfLength + Query.Radius;
				return true;
			}
		}

		return false;
	}

	// check ------------------------------------------
	
	bool CheckCircle(const FSpTargetQuery& Query, const FVector& TargetLocation, const float TargetRadius)
	{
		const float HitRadius = FMath::Max(Query.Radius, 0.0f) + TargetRadius;
		return FVector::DistSquared2D(Query.Origin, TargetLocation) <= FMath::Square(HitRadius);
	}

	bool CheckSegment(const FVector& Start, const FVector& End, const float Radius, const FVector& TargetLocation, const float TargetRadius, float& OutHitTime)
	{
		return TryGetSegmentCircleHitTime(Start, End, TargetLocation, FMath::Max(Radius, 0.0f) + TargetRadius, OutHitTime);
	}

	bool CheckSector(const FSpTargetQuery& Query, const FVector& TargetLocation, const float TargetRadius)
	{
		FVector ToTarget = TargetLocation - Query.Origin;
		ToTarget.Z = 0.0f;

		const float DistanceSquared = ToTarget.SizeSquared2D();
		if (DistanceSquared <= UE_KINDA_SMALL_NUMBER)
			return true;

		const float HitRadius = Query.Radius + TargetRadius;
		if (DistanceSquared > FMath::Square(HitRadius))
			return false;

		const float Distance = FMath::Sqrt(DistanceSquared);
		const float Dot = FVector::DotProduct(GetDirection2D(Query.Direction), ToTarget / Distance);
		const float TargetAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
		const float HalfAngle = Query.AngleDegrees * 0.5f;
		if (TargetAngle <= HalfAngle)
			return true;

		const float AnglePadding = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(TargetRadius / Distance, 0.0f, 1.0f)));
		return TargetAngle <= HalfAngle + AnglePadding;
	}

	bool CheckBox(const FSpTargetQuery& Query, const FVector& TargetLocation, const float TargetRadius)
	{
		FVector ToTarget = TargetLocation - Query.Origin;
		ToTarget.Z = 0.0f;

		const FVector Forward = GetDirection2D(Query.Direction);
		const FVector Side(-Forward.Y, Forward.X, 0.0f);
		const float ForwardDistance = FVector::DotProduct(ToTarget, Forward);
		const float SideDistance = FVector::DotProduct(ToTarget, Side);
		const float HalfWidth = Query.Width * 0.5f;

		const float ClampedForward = FMath::Clamp(ForwardDistance, 0.0f, Query.Length);
		const float ClampedSide = FMath::Clamp(SideDistance, -HalfWidth, HalfWidth);
		const float ForwardDelta = ForwardDistance - ClampedForward;
		const float SideDelta = SideDistance - ClampedSide;
		return ForwardDelta * ForwardDelta + SideDelta * SideDelta <= FMath::Square(TargetRadius);
	}

	bool CheckShape(const FSpTargetQuery& Query, const ASpUnit& Candidate, float& OutSegmentHitTime)
	{
		OutSegmentHitTime = 0.0f;
		const FVector TargetLocation = Candidate.GetActorLocation();
		const float TargetRadius = Query.bIncludeTargetCollisionRadius ? GetCollisionRadius(Candidate) : 0.0f;

		switch (Query.Shape)
		{
			case ESpTargetShape::Circle:
				return CheckCircle(Query, TargetLocation, TargetRadius);

			case ESpTargetShape::Line:
			{
				const FVector End = Query.Origin + GetDirection2D(Query.Direction) * Query.Length;
				return CheckSegment(Query.Origin, End, Query.Radius, TargetLocation, TargetRadius, OutSegmentHitTime);
			}

			case ESpTargetShape::Sector:
				return CheckSector(Query, TargetLocation, TargetRadius);

			case ESpTargetShape::Box:
				return CheckBox(Query, TargetLocation, TargetRadius);

			case ESpTargetShape::Segment:
				return CheckSegment(Query.Origin, Query.SegmentEnd, Query.Radius, TargetLocation, TargetRadius, OutSegmentHitTime);
		}

		return false;
	}

	// ------------------------------------------------
	
	bool CollectTargetsInternal(const UWorld* World, const ASpUnit* Source, uint32 SourceUnitUid, FGenericTeamId SourceTeamId, const FSpTargetQuery& Query, TArray<ASpUnit*>& OutTargets)
	{
		OutTargets.Reset();

		if (!World || World->GetNetMode() == NM_Client || !Query.IsValid())
			return false;

		const UUnitRegistrySubsystem* UnitRegistry = UUnitRegistrySubsystem::Get(World);
		if (!UnitRegistry)
			return false;

		FVector SearchCenter;
		float SearchRadius = 0.0f;
		if (!GetSearchBounds(Query, SearchCenter, SearchRadius))
			return false;

		TArray<ASpUnit*> Candidates;
		UnitRegistry->GetUnitsInRange(SearchCenter, SearchRadius, Candidates, Query.bIncludeTargetCollisionRadius);

		TArray<FCollectedTarget> CollectedTargets;
		CollectedTargets.Reserve(Candidates.Num());

		for (ASpUnit* Candidate : Candidates)
		{
			if (!IsValid(Candidate) || !IsValidCandidate(Source, SourceUnitUid, SourceTeamId, *Candidate, Query))
				continue;

			float SegmentHitTime = 0.0f;
			if (!CheckShape(Query, *Candidate, SegmentHitTime))
				continue;

			FCollectedTarget& Collected = CollectedTargets.AddDefaulted_GetRef();
			Collected.Target = Candidate;
			Collected.DistanceSquared = FVector::DistSquared2D(Query.Origin, Candidate->GetActorLocation());
			Collected.SegmentHitTime = SegmentHitTime;
		}

		switch (Query.Sort)
		{
			case ESpTargetSort::Distance:
				CollectedTargets.StableSort([](const FCollectedTarget& Lhs, const FCollectedTarget& Rhs)
				{
					return Lhs.DistanceSquared < Rhs.DistanceSquared;
				});
				break;

			case ESpTargetSort::SegmentHitTime:
				CollectedTargets.StableSort([](const FCollectedTarget& Lhs, const FCollectedTarget& Rhs)
				{
					return Lhs.SegmentHitTime < Rhs.SegmentHitTime;
				});
				break;

			default:
				break;
		}

		if (Query.MaxTargets > 0 && CollectedTargets.Num() > Query.MaxTargets)
			CollectedTargets.SetNum(Query.MaxTargets);

		OutTargets.Reserve(CollectedTargets.Num());
		for (const FCollectedTarget& Collected : CollectedTargets)
		{
			if (IsValid(Collected.Target))
				OutTargets.Add(Collected.Target);
		}

		return !OutTargets.IsEmpty();
	}
}

// ==================================================

bool USpTargetCollectorLibrary::CollectTargets(const ASpUnit* Source, const FSpTargetQuery& Query, TArray<ASpUnit*>& OutTargets)
{
	if (!IsValid(Source) || !Source->HasAuthority())
	{
		OutTargets.Reset();
		return false;
	}

	return CollectTargetsInternal(Source->GetWorld(), Source, Source->GetUnitUid(), Source->GetGenericTeamId(), Query, OutTargets);
}

bool USpTargetCollectorLibrary::CollectTargetsForField(const UWorld* World, uint32 SourceUnitUid, FGenericTeamId SourceTeamId, const FSpTargetQuery& Query, TArray<ASpUnit*>& OutTargets)
{
	return CollectTargetsInternal(World, nullptr, SourceUnitUid, SourceTeamId, Query, OutTargets);
}
