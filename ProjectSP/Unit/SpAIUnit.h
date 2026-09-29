#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Subsystem/ActorPool/SpPoolableActor.h"
#include "SpAIUnit.generated.h"

class USpUnitStimuliSourceComponent;
class USpUnitAggroComponent;

// ==================================================

UCLASS()
class PROJECTSP_API ASpAIUnit : public ASpUnit, public ISpPoolableActor
{
	GENERATED_BODY()

public:
	ASpAIUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Push() override;
	virtual void OnDestroy() override;
	virtual void OnReturn() override;

protected:
	virtual void HandleDeadProcessFinished_Server() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitAggroComponent> AggroComponent;
};
