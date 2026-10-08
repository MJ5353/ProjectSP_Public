#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpAIUnitDefinition.generated.h"

class UAggroDefinition;
class ASpAIController;
class UBehaviorTree;

// ==================================================

UCLASS()
class PROJECTSP_API USpAIDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ASpAIController> AIControllerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBehaviorTree> BehaviorTree;
};
