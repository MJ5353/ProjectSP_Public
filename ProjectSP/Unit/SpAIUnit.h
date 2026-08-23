#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "SpAIUnit.generated.h"

class USpUnitStimuliSourceComponent;
class USpUnitAggroComponent;

// ==================================================

UCLASS()
class PROJECTSP_API ASpAIUnit : public ASpUnit
{
	GENERATED_BODY()

public:
	ASpAIUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitAggroComponent> AggroComponent;
};
