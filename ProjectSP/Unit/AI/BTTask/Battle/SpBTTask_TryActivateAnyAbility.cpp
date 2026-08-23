#include "SpBTTask_TryActivateAnyAbility.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/AI/SpEnemyAIController.h"

// ==================================================

USpBTTask_TryActivateAnyAbility::USpBTTask_TryActivateAnyAbility()
{
	NodeName = TEXT("Try Activate Any Ability");
}

bool USpBTTask_TryActivateAnyAbility::CheckTargetCondition(const UBlackboardComponent* BlackboardComponent, const ASpUnit* OwnerUnit, const float ExecuteRange)
{
	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor));
	if (!IsValid(TargetActor))
		return false;

	FVector ToTarget = TargetActor->GetActorLocation() - OwnerUnit->GetActorLocation();
	ToTarget.Z = 0.0f;

	if (!ToTarget.IsNearlyZero())
	{
		const float TargetYaw = ToTarget.Rotation().Yaw;
		const float CurrentYaw = OwnerUnit->GetActorRotation().Yaw;
		const float YawDifference = FMath::Abs(FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw));
			
		if (YawDifference > SpEnemyAI::Define::FacingToleranceDegrees)
			return false;
	}

	float ExecuteRangeSq = FMath::Square(ExecuteRange + 100.f);
	float DistanceSq = ToTarget.SizeSquared2D();
		
	return ExecuteRangeSq >= DistanceSq;
}

EBTNodeResult::Type USpBTTask_TryActivateAnyAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	ASpEnemyAIController* EnemyController = Cast<ASpEnemyAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	
	if (!EnemyController || !BlackboardComponent)
		return EBTNodeResult::Failed;

	BlackboardComponent->SetValueAsFloat(SpEnemyAI::BlackboardKeys::TargetFacingRange, SpEnemyAI::Define::FocusRange);

	ASpUnit* OwnerUnit = EnemyController->GetPawn<ASpUnit>();
	if (!OwnerUnit)
		return EBTNodeResult::Failed;
	
	USpAbilitySystemComponent* SpASC = OwnerUnit->GetSpAbilitySystemComponent();
	if (!SpASC)
		return  EBTNodeResult::Failed;
	
	FGameplayTag AbilityTag;
	bool bNeedTarget;
	float ExecuteRange;
	
	if (!EnemyController->TryGetActivatableAbility(AbilityTag, bNeedTarget, ExecuteRange))
		return EBTNodeResult::Failed;
	
	BlackboardComponent->SetValueAsFloat(SpEnemyAI::BlackboardKeys::TargetFacingRange, ExecuteRange);
	
	if (bNeedTarget && !CheckTargetCondition(BlackboardComponent, OwnerUnit, ExecuteRange))
		return EBTNodeResult::Failed;
	
	if (!SpASC->TryActivateAbilityByTag(AbilityTag))
		return EBTNodeResult::Failed;
	
	return EBTNodeResult::Succeeded;
}
