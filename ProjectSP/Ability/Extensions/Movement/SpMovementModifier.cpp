#include "SpMovementModifier.h"

// ==================================================

void USpMovementModifier::AccumulateDistance(float& InOutAdditionalDistance, float& InOutMultiplier) const
{
	InOutAdditionalDistance += AdditionalDistance;
	InOutMultiplier *= FMath::Max(DistanceMultiplier, 0.0f);
}
