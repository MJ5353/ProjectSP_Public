#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandTypes.h"

class ASpUnit;
class FSpPlayerCommandService_Movement;

// ==================================================

// PrimaryAttack만의 사거리와 추적 재경로 규칙.
class FSpPlayerPrimaryAttackRules
{
	float AttackRange = 200.0f;
	float ChaseRepathDistance = 100.0f;

public:
	FSpPlayerPrimaryAttackRules() = default;
	FSpPlayerPrimaryAttackRules(float InAttackRange, float InChaseRepathDistance)
		: AttackRange(InAttackRange), ChaseRepathDistance(InChaseRepathDistance)
	{
	}

	bool IsTargetInRange(const ASpUnit& SourceUnit, const ASpUnit& TargetUnit) const;
	bool NeedsChasePath(const FSpPlayerCommandService_Movement& MovementService, const FVector& TargetLocation) const;
};

// ==================================================

class FSpPlayerPrimaryAttackCommand final : public ISpPlayerCommand
{
	uint16 CommandId = 0;
	TWeakObjectPtr<ASpUnit> TargetUnit;
	FSpPlayerPrimaryAttackRules Rules;

public:
	FSpPlayerPrimaryAttackCommand(uint16 InCommandId, ASpUnit* InTargetUnit, FSpPlayerPrimaryAttackRules InRules);

	virtual uint16 GetId() const override { return CommandId; }
	virtual bool ShouldEndOnInputEnd() const override { return true; }

	virtual AActor* GetTargetActor() const override;
	virtual bool Begin(FSpPlayerCommandContext_Server& Context) override;
	virtual bool Update(FSpPlayerCommandContext_Server& Context, const FSpPlayerCommandResolvedInput& Input) override;
	virtual bool Tick(FSpPlayerCommandContext_Server& Context, float DeltaTime) override;
	virtual ESpPlayerCommandAbilityValidation ValidateAbility(FSpPlayerCommandContext_Server& Context, FGameplayTag AbilityTag) override;
};
