#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "SpBTService_RotateToTarget.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpBTService_RotateToTarget : public UBTService
{
	GENERATED_BODY()
	
public:
	USpBTService_RotateToTarget();
	
private:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
