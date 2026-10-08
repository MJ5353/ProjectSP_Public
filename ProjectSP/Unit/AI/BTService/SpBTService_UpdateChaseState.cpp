#include "SpBTService_UpdateChaseState.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectSP/Unit/SpUnit.h"
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

	const ASpUnit* Unit = Controller->GetPawn<ASpUnit>();
	const ASpUnit* TargetActor = Unit ? Unit->GetTargetActor() : nullptr;
	if (Unit && Unit->IsAttackable(TargetActor))
	{
		const bool bCanChase = Controller->CheckNeedChase(TargetActor->GetActorLocation());
		Blackboard->SetValueAsBool(SpEnemyAI::BlackboardKeys::CanChaseTarget, bCanChase);
		
		return;
	}

	Blackboard->SetValueAsBool(SpEnemyAI::BlackboardKeys::CanChaseTarget, false);
}
