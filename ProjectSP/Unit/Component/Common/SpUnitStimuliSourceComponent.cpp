#include "SpUnitStimuliSourceComponent.h"
#include "Perception/AISense_Sight.h"

// ==================================================

void USpUnitStimuliSourceComponent::OnInitUnit()
{
	RegisterAsSourceForSenses.AddUnique(UAISense_Sight::StaticClass());
}

void USpUnitStimuliSourceComponent::OnClearUnit()
{
	UnregisterFromPerceptionSystem();
}

void USpUnitStimuliSourceComponent::OnUnitPlayable(bool bPlayable)
{
	if (bPlayable)
		RegisterWithPerceptionSystem();
	else
		UnregisterFromPerceptionSystem();
}
