#pragma once

#include "Misc/Optional.h"
#include "ProjectSP/Input/SpInputBehavior.h"
#include "SpPrimaryInputBehavior.generated.h"

class ASpUnit;

// ==================================================

UCLASS()
class PROJECTSP_API USpPrimaryInputBehavior : public USpInputBehavior
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "MJ - Input", meta = (ClampMin = "0.0", Units = "cm"))
	float TargetSelectionRadius = 30.0f;

	TOptional<FVector> Destination;
	FGameplayTag AbilityTag;
	bool bAbilityActivationPending = false;
	bool bAbilityActivated = false;
	uint64 StateRevision = 0;

public:
	virtual ESpInputBehaviorRequest HandleInput(ASpPlayerUnit& Unit, const FInputActionValue& Value, ETriggerEvent TriggerEvent, const FSpInputActionBinding& Binding, bool bIsActiveBehavior) override;
	virtual void TickBehavior(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, FGameplayTag SourceInputTag, float DeltaTime) override;
	virtual void ClearBehavior(ASpPlayerUnit* Unit) override;

private:
	// process
	bool ProcessPendingAbility(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, FGameplayTag SourceInputTag, float DeltaTime);
	bool TryActivatePendingAbility(ASpPlayerUnit& Unit, USpPlayerActionComponent& ActionComponent, FGameplayTag SourceInputTag);
	bool ProcessTargetDistance(ASpPlayerUnit& Unit, const ASpUnit& Target, float ExecuteRange);
	bool ProcessTargetFacing(ASpPlayerUnit& Unit, const ASpUnit& Target, float DeltaTime);
	
	// handle
	ESpInputBehaviorRequest HandleInputBegin(ASpPlayerUnit& Unit, const FSpInputActionBinding& Binding);
	ESpInputBehaviorRequest HandleInputTriggered(ASpPlayerUnit& Unit, const FSpInputActionBinding& Binding);
	ESpInputBehaviorRequest HandleInputEnd(ASpPlayerUnit& Unit);
};
