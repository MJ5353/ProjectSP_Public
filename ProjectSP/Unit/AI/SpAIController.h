#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Subsystem/ActorPool/SpPoolableActor.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpAIController.generated.h"

class USpAIDefinition;

// ==================================================

UCLASS()
class PROJECTSP_API ASpAIController : public AAIController, public ISpUnitManageListener, public ISpPoolableActor
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "MJ - Runtime", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTree> BehaviorTree;

	// Preparing 단계에서는 Behavior Tree를 시작하지 않기 위함
	bool bUnitGameplayActive = false;
	
public:
	virtual void SetDefinition(USpAIDefinition* Definition);
	
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnitActive(bool bActive) override;
	virtual void Push() override;
	
	void TryStartBehaviorTree();
};
