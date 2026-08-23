#pragma once

#include "CoreMinimal.h"
#include "SpPlayerPrimaryAttackCommand.h"

class ASpUnit;
class USpAbilitySystemComponent;

// ==================================================

class FSpPlayerPrimaryAttackPrediction final : public ISpPlayerCommandPrediction
{
	uint16 CommandId = 0;
	TWeakObjectPtr<ASpUnit> SourceUnit;
	TWeakObjectPtr<ASpUnit> TargetUnit;
	TWeakObjectPtr<USpAbilitySystemComponent> AbilitySystemComponent;
	FSpPlayerPrimaryAttackRules Rules;
	float NextPredictionTime = 0.0f;

public:
	FSpPlayerPrimaryAttackPrediction(uint16 InCommandId, ASpUnit* InSourceUnit, ASpUnit* InTargetUnit, USpAbilitySystemComponent* InAbilitySystemComponent, FSpPlayerPrimaryAttackRules InRules);

	virtual uint16 GetId() const override { return CommandId; }
	virtual bool Tick(float DeltaTime) override;
	virtual void Cancel() override {}
};
