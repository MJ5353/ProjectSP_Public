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

	// 이미 귀환 위치가 잡혀 있으면, 귀환 시퀀스가 이미 진행중인 것으로 판정
	if (HomeLocation != FVector::ZeroVector)
		return EBTNodeResult::Failed;

	// 아직 귀환할 필요가 없으면 Return Home 분기를 실패시켜 Patrol로 넘긴다.
	if (!EnemyController->CheckNeedReturn(HomeLocation))
		return EBTNodeResult::Failed;
	
	BlackboardComponent->SetValueAsVector(SpEnemyAI::BlackboardKeys::HomeLocation, HomeLocation);
	return EBTNodeResult::Succeeded;
}
