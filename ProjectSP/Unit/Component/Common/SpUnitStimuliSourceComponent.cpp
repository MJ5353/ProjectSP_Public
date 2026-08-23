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

void USpUnitStimuliSourceComponent::OnUnitActive(bool bActive)
{
	if (bActive)
		RegisterWithPerceptionSystem();
	else
		UnregisterFromPerceptionSystem();
}
