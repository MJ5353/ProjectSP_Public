#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SpBTTask_TrySetReturn.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpBTTask_TrySetReturn : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	USpBTTask_TrySetReturn();
	
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
