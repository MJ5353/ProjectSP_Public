#include "SpBTTask_TrySetReturn.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/ValueOrBBKey.h"
#include "ProjectSP/Unit/AI/SpEnemyAIController.h"

// ==================================================

USpBTTask_TrySetReturn::USpBTTask_TrySetReturn()
{
	NodeName = TEXT("Try Set Return");
	bNotifyTick = true;
}

EBTNodeResult::Type USpBTTask_TrySetReturn::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASpEnemyAIController* EnemyController = Cast<ASpEnemyAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();

	if (!EnemyController || !BlackboardComponent)
		return EBTNodeResult::Failed;

	FVector HomeLocation = BlackboardComponent->GetValueAsVector(SpEnemyAI::BlackboardKeys::HomeLocation);
	if (HomeLocation != FVector::ZeroVector)
		return EBTNodeResult::Failed;

	if (!EnemyController->CheckNeedReturn(HomeLocation))
		return EBTNodeResult::Failed;
	
	BlackboardComponent->SetValueAsVector(SpEnemyAI::BlackboardKeys::HomeLocation, HomeLocation);
	return EBTNodeResult::Succeeded;
}
