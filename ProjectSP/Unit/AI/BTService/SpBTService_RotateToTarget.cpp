#include "SpBTService_RotateToTarget.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/AI/SpEnemyAIController.h"

// ==================================================

USpBTService_RotateToTarget::USpBTService_RotateToTarget()
{
	NodeName = "Rotate To Target";
	bNotifyTick = true;
}

void USpBTService_RotateToTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ASpEnemyAIController* EnemyController = Cast<ASpEnemyAIController>(OwnerComp.GetAIOwner());
	if (!EnemyController)
		return;

	ASpUnit* Unit = EnemyController->GetPawn<ASpUnit>();
	if (!Unit)
		return;

	ASpUnit* TargetActor = Unit->GetTargetActor();
	if (!Unit->IsAttackable(TargetActor))
		return;
	
	FVector TargetLocation = TargetActor->GetActorLocation();
	Unit->RotateToTargetLocation(TargetLocation, DeltaSeconds, 1.f);
}
