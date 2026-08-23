#include "SpBTService_UpdateChaseState.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectSP/Unit/AI/SpEnemyAIController.h"

// ==================================================

USpBTService_UpdateChaseState::USpBTService_UpdateChaseState()
{
	NodeName = "Update Chase Node";
	bNotifyTick = true;
}

void USpBTService_UpdateChaseState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	ASpEnemyAIController* Controller = Cast<ASpEnemyAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Blackboard)
		return;

	Controller->RefreshCombatTarget();

	if (UObject* TargetObject = Blackboard->GetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor); 
		AActor* TargetActor = Cast<AActor>(TargetObject))
	{
		const bool bCanChase = Controller->CheckNeedChase(TargetActor->GetActorLocation());
		Blackboard->SetValueAsBool(SpEnemyAI::BlackboardKeys::CanChaseTarget, bCanChase);
		
		return;
	}

	Blackboard->SetValueAsBool(SpEnemyAI::BlackboardKeys::CanChaseTarget, false);
}
