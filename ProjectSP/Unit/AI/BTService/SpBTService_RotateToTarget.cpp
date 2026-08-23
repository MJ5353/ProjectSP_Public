#include "SpBTService_RotateToTarget.h"
#include "BehaviorTree/BlackboardComponent.h"
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
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	
	if (!EnemyController || !BlackboardComponent)
		return;
	
	UObject* TargetObject = BlackboardComponent->GetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor);
	if (!TargetObject)
		return;
	
	AActor* TargetActor = Cast<AActor>(TargetObject);
	if (!IsValid(TargetActor))
		return;
	
	ASpUnit* Unit = EnemyController->GetPawn<ASpUnit>();
	if (!Unit)
		return;
	
	FVector TargetLocation = TargetActor->GetActorLocation();
	Unit->RotateToTargetLocation(TargetLocation, DeltaSeconds, 1.f);
}