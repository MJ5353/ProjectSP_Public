#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "SpBTService_UpdateChaseState.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API USpBTService_UpdateChaseState : public UBTService
{
	GENERATED_BODY()
	
public:
	USpBTService_UpdateChaseState();
	
private:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
