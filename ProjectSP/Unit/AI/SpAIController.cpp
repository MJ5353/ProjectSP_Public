#include "SpAIController.h"
#include "BrainComponent.h"
#include "ProjectSP/Definition/Unit/AI/SpAIUnitDefinition.h"

// ==================================================

void ASpAIController::SetDefinition(USpAIDefinition* Definition)
{
	if (!Definition)
	{
		if (UBrainComponent* Brain = GetBrainComponent())
			Brain->StopLogic(TEXT("AI definition is nullptr"));

		BehaviorTree = nullptr;
		return;
	}

	BehaviorTree = Definition->BehaviorTree;
	TryStartBehaviorTree();
}

void ASpAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	TryStartBehaviorTree();
}

void ASpAIController::OnUnitPlayable(bool bPlayable)
{
	bUnitGameplayActive = bPlayable;

	if (bUnitGameplayActive)
	{
		TryStartBehaviorTree();
		return;
	}

	if (UBrainComponent* Brain = GetBrainComponent())
		Brain->StopLogic(TEXT("Unit returned to pool"));
}

void ASpAIController::Push()
{
}

void ASpAIController::TryStartBehaviorTree()
{
	if (!bUnitGameplayActive || !GetPawn() || !BehaviorTree)
		return;
	
	RunBehaviorTree(BehaviorTree);
}
