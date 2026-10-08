#include "SpBTTask_TryActivateAnyAbility.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/AI/SpEnemyAIController.h"

// ==================================================

USpBTTask_TryActivateAnyAbility::USpBTTask_TryActivateAnyAbility()
{
	NodeName = TEXT("Try Activate Any Ability");
}

bool USpBTTask_TryActivateAnyAbility::CheckTargetCondition(const ASpUnit* OwnerUnit, const float AllowedRange, const bool bNeedTargetFacing)
{
	const ASpUnit* TargetActor = OwnerUnit->GetTargetActor();
	if (!OwnerUnit->IsAttackable(TargetActor))
		return false;

	FVector ToTarget = TargetActor->GetActorLocation() - OwnerUnit->GetActorLocation();
	ToTarget.Z = 0.0f;

	if (bNeedTargetFacing && !OwnerUnit->IsFacingTargetLocation(TargetActor->GetActorLocation()))
		return false;

	// check dist
	float ExecuteRangeSq = FMath::Square(AllowedRange);
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
	
	const bool bFoundAbility = EnemyController->TryGetActivatableAbility(AbilityTag, bNeedTarget, ExecuteRange);
	const float AllowedRange = bNeedTarget ? ExecuteRange + SpEnemyAI::Define::TargetRangeTolerance : ExecuteRange;
	BlackboardComponent->SetValueAsFloat(SpEnemyAI::BlackboardKeys::TargetFacingRange, AllowedRange);
	
	if (!bFoundAbility)
		return EBTNodeResult::Failed;
	
	const bool bNeedTargetFacing = SpASC->HasAbilityAssetTag(AbilityTag, SpGameplayTags::AbilityConditionTag_TargetFacing);
	if (bNeedTarget && !CheckTargetCondition(OwnerUnit, AllowedRange, bNeedTargetFacing))
		return EBTNodeResult::Failed;
	
	if (!SpASC->TryActivateAbilityByTag(AbilityTag))
		return EBTNodeResult::Failed;
	
	return EBTNodeResult::Succeeded;
}
