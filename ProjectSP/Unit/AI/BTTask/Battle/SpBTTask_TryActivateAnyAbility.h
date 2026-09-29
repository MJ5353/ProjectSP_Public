#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "SpBTTask_TryActivateAnyAbility.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpBTTask_TryActivateAnyAbility : public UBTTaskNode
{
	GENERATED_BODY()

public:
	USpBTTask_TryActivateAnyAbility();
	bool CheckTargetCondition(const ASpUnit* OwnerUnit, float AllowedRange, bool bNeedTargetFacing);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
