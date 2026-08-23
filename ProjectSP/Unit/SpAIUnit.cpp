#include "SpAIUnit.h"
#include "Component/Common/SpUnitAggroComponent.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"

// ==================================================

ASpAIUnit::ASpAIUnit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<USpAbilitySystemComponent>(TEXT("SpAbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AggroComponent = CreateDefaultSubobject<USpUnitAggroComponent>(TEXT("SpUnitAggroComponent"));
}
