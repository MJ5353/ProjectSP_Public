#include "SpPlayerPrimaryAttackPrediction.h"

#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Unit/SpUnit.h"

namespace
{
	constexpr float PredictionRetryInterval = 0.1f;
}

// ==================================================

FSpPlayerPrimaryAttackPrediction::FSpPlayerPrimaryAttackPrediction(const uint16 InCommandId, ASpUnit* InSourceUnit, ASpUnit* InTargetUnit, USpAbilitySystemComponent* InAbilitySystemComponent, FSpPlayerPrimaryAttackRules InRules)
	: CommandId(InCommandId)
	, SourceUnit(InSourceUnit)
	, TargetUnit(InTargetUnit)
	, AbilitySystemComponent(InAbilitySystemComponent)
	, Rules(MoveTemp(InRules))
{
}

bool FSpPlayerPrimaryAttackPrediction::Tick(const float DeltaTime)
{
	ASpUnit* Unit = SourceUnit.Get();
	ASpUnit* Target = TargetUnit.Get();
	USpAbilitySystemComponent* ASC = AbilitySystemComponent.Get();
	ASpPlayerController* PlayerController = Unit ? Unit->GetController<ASpPlayerController>() : nullptr;
	
	bool bValidParam = !Unit || !Target || !ASC || !PlayerController;
	if (bValidParam || !PlayerController->IsGameplayInputEnabled_Client() || !Target->IsAttackable(Unit))
		return false;

	UWorld* World = Unit->GetWorld();
	if (!World || World->GetTimeSeconds() < NextPredictionTime || !Rules.IsTargetInRange(*Unit, *Target))
		return true;

	const FGameplayTag BasicAttackTag = FSpGameplayTags::Get().AbilitySlotTag_BasicAttack;
	if (ASC->IsAbilityTagActive(BasicAttackTag))
		return true;

	float CooldownRemaining = 0.0f;
	float CooldownDuration = 0.0f;
	
	if (ASC->GetAbilityCooldownByTag(BasicAttackTag, CooldownRemaining, CooldownDuration) && CooldownRemaining > 0.0f)
	{
		NextPredictionTime = World->GetTimeSeconds() + CooldownRemaining;
		return true;
	}

	NextPredictionTime = World->GetTimeSeconds() + PredictionRetryInterval;
	ASC->TryActivateAbilityForPlayerCommand_Client(BasicAttackTag, CommandId);
	
	return true;
}
