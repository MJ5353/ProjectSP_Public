#include "SpBTTask_TrySetPatrol.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectSP/Unit/AI/SpEnemyAIController.h"

// ==================================================

USpBTTask_TrySetPatrol::USpBTTask_TrySetPatrol()
{
	NodeName = TEXT("Try Set Patrol");
}

EBTNodeResult::Type USpBTTask_TrySetPatrol::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASpEnemyAIController* EnemyController = Cast<ASpEnemyAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	
	if (!EnemyController || !BlackboardComponent)
		return EBTNodeResult::Failed;
	
	FVector PatrolLocation = BlackboardComponent->GetValueAsVector(SpEnemyAI::BlackboardKeys::PatrolLocation);
	if (PatrolLocation != FVector::ZeroVector)
		return EBTNodeResult::Succeeded;
	
	if (!EnemyController->TryGetRandomLocationInHomeRange(PatrolLocation))
		return EBTNodeResult::Failed;

	BlackboardComponent->SetValueAsVector(SpEnemyAI::BlackboardKeys::PatrolLocation, PatrolLocation);
	return EBTNodeResult::Succeeded;
}
