#include "SpAreaTrailModifier.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "ProjectSP/Ability/Core/SpGameplayAbility.h"

// ==================================================

void USpAreaTrailModifier::OnMovementStarted(USpGameplayAbility* Ability, const FVector& StartLocation)
{
	DistanceTraveled = 0.0f;
	NextPlacementDistance = FMath::Max(AreaSpacing, 1.0f);

	if (bPlaceAtStart)
		PlaceArea(Ability, StartLocation);
}

void USpAreaTrailModifier::OnMovementStep(USpGameplayAbility* Ability, const FVector& PreviousLocation, const FVector& CurrentLocation)
{
	const float SegmentDistance = FVector::Dist(PreviousLocation, CurrentLocation);
	if (SegmentDistance <= KINDA_SMALL_NUMBER)
		return;

	const float PreviousDistance = DistanceTraveled;
	DistanceTraveled += SegmentDistance;

	while (NextPlacementDistance <= DistanceTraveled)
	{
		const float Alpha = (NextPlacementDistance - PreviousDistance) / SegmentDistance;
		PlaceArea(Ability, FMath::Lerp(PreviousLocation, CurrentLocation, Alpha));
		
		// 일정 간격
		NextPlacementDistance += FMath::Max(AreaSpacing, 1.0f);
	}
}

void USpAreaTrailModifier::PlaceArea(USpGameplayAbility* Ability, const FVector& Location) const
{
	AActor* Avatar = Ability ? Ability->GetAvatarActorFromActorInfo() : nullptr;
	if (!DamageAreaClass || !IsValid(Avatar) || !Avatar->HasAuthority())
		return;

	UWorld* World = Avatar->GetWorld();
	if (!World)
		return;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = Avatar;
	SpawnParameters.Instigator = Cast<APawn>(Avatar);
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<AActor>(DamageAreaClass, Location, Avatar->GetActorRotation(), SpawnParameters);
}
