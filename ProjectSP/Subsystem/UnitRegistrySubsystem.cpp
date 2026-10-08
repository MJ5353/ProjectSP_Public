#include "UnitRegistrySubsystem.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "GenericTeamAgentInterface.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

UUnitRegistrySubsystem* UUnitRegistrySubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UUnitRegistrySubsystem>() : nullptr;
}

// clear

void UUnitRegistrySubsystem::OnWorldEndPlay(UWorld& InWorld)
{
	Clear();
	Super::OnWorldEndPlay(InWorld);
}

void UUnitRegistrySubsystem::Clear()
{
	// ClearUnit()의 콜백에서 등록 목록이 변경될 수 있으므로 먼저 복사
	const TArray<TObjectPtr<ASpUnit>> RegisteredUnits = Units;
	for (ASpUnit* Unit : RegisteredUnits)
	{
		if (IsValid(Unit))
			Unit->ClearUnit();
	}

	Units.Empty();
	UnitsByTeam.Empty();
}

// core

void UUnitRegistrySubsystem::RegisterUnit(ASpUnit* Unit)
{
	if (!IsValid(Unit))
		return;

	Units.AddUnique(Unit);
	RemoveUnitFromTeamLists(Unit);
	AddUnitToTeam(Unit);
}

void UUnitRegistrySubsystem::UnregisterUnit(ASpUnit* Unit)
{
	Units.RemoveAll([Unit](const TObjectPtr<ASpUnit>& UnitPtr)
	{
		return !IsValid(UnitPtr.Get()) || UnitPtr.Get() == Unit;
	});

	RemoveUnitFromTeamLists(Unit);
}

void UUnitRegistrySubsystem::RefreshUnitTeam(ASpUnit* Unit)
{
	if (!IsValid(Unit) || !IsUnitRegistered(Unit))
		return;

	RemoveUnitFromTeamLists(Unit);
	AddUnitToTeam(Unit);
}

void UUnitRegistrySubsystem::AddUnitToTeam(ASpUnit* Unit)
{
	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Unit);
	if (!TeamAgent)
		return;

	const uint8 Key = TeamAgent->GetGenericTeamId().GetId();
	
	TArray<TObjectPtr<ASpUnit>>& TeamUnits = UnitsByTeam.FindOrAdd(Key);
	TeamUnits.AddUnique(Unit);
}

void UUnitRegistrySubsystem::RemoveUnitFromTeamLists(ASpUnit* Unit)
{
	for (auto It = UnitsByTeam.CreateIterator(); It; ++It)
	{
		TArray<TObjectPtr<ASpUnit>>& TeamUnits = It.Value();
		TeamUnits.RemoveAll([Unit](const TObjectPtr<ASpUnit>& UnitPtr)
		{
			return !IsValid(UnitPtr.Get()) || UnitPtr.Get() == Unit;
		});

		if (TeamUnits.IsEmpty())
			It.RemoveCurrent();
	}
}

// get

ASpUnit* UUnitRegistrySubsystem::FindNearestUnit(const AActor* Querier, const float Radius, const ETeamAttitude::Type Attitude)
{
	if (!Querier)
		return nullptr;
	
	const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Querier);
	if (!TeamAgent)
		return nullptr;

	float BestDistanceSq = FMath::Square(Radius);
	ASpUnit* BestUnit = nullptr;

	for (const auto& TeamPair : UnitsByTeam)
	{
		for (ASpUnit* Candidate : TeamPair.Value)
		{
			if (!IsValid(Candidate) || Candidate == Querier)
				continue;

			if (TeamAgent->GetTeamAttitudeTowards(*Candidate) != Attitude)
				continue;

			const float DistanceSq = FVector::DistSquared2D(Querier->GetActorLocation(), Candidate->GetActorLocation());

			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestUnit = Candidate;
			}
		}
	}

	return BestUnit;
}

void UUnitRegistrySubsystem::GetUnitsInRange(const FVector& Center, const float Radius, TArray<ASpUnit*>& OutUnits, const bool bIncludeCollisionRadius) const
{
	OutUnits.Reset();
	
	if (Radius < 0.0f)
		return;

	for (ASpUnit* Unit : Units)
	{
		if (!IsValid(Unit))
			continue;

		const UCapsuleComponent* Capsule = bIncludeCollisionRadius ? Unit->GetCapsuleComponent() : nullptr;
		const float UnitRadius = Capsule ? FMath::Max(Capsule->GetScaledCapsuleRadius(), 0.0f) : 0.0f;
		const float EffectiveRadius = Radius + UnitRadius;
		if (FVector::DistSquared2D(Center, Unit->GetActorLocation()) <= FMath::Square(EffectiveRadius))
			OutUnits.Add(Unit);
	}
}

bool UUnitRegistrySubsystem::IsUnitRegistered(const ASpUnit* Unit) const
{
	for (const ASpUnit* RegisteredUnit : Units)
	{
		if (RegisteredUnit == Unit)
			return true;
	}

	return false;
}

void UUnitRegistrySubsystem::GetTeamUnitsToIgnoreCollision(FGenericTeamId TeamId, TArray<AActor*>& OutActorsToIgnore) const
{
	OutActorsToIgnore.Reset();
	const uint8 Key = TeamId.GetId();

	const TArray<TObjectPtr<ASpUnit>>* TeamUnits = UnitsByTeam.Find(Key);
	if (!TeamUnits)
		return;

	for (ASpUnit* Unit : *TeamUnits)
	{
		if (IsValid(Unit))
			OutActorsToIgnore.Add(Unit);
	}
}

void UUnitRegistrySubsystem::GetTeamUnitsToIgnoreCollision(const FGenericTeamId& TeamId, FCollisionQueryParams& Params) const
{
	const uint8 Key = TeamId.GetId();

	const TArray<TObjectPtr<ASpUnit>>* TeamUnits = UnitsByTeam.Find(Key);
	if (!TeamUnits)
		return;

	for (const ASpUnit* Unit : *TeamUnits)
	{
		if (IsValid(Unit))
			Params.AddIgnoredActor(Unit);
	}
}
