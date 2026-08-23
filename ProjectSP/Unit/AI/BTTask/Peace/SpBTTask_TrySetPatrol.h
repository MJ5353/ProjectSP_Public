#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "SpBTTask_TrySetPatrol.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpBTTask_TrySetPatrol : public UBTTaskNode
{
	GENERATED_BODY()
	
protected:
	USpBTTask_TrySetPatrol();
	
public:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
