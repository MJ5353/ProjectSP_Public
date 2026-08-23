#pragma once

#include "CoreMinimal.h"
#include "SpAIController.h"
#include "SpEnemyAIController.generated.h"

struct FAIStimulus;
class UAISenseConfig_Sight;
class USpEnemyAIDefinition;
class USpUnitAggroComponent;

// ==================================================

namespace SpEnemyAI::BlackboardKeys
{
	inline const FName TargetActor(TEXT("TargetActor"));
	inline const FName HomeLocation(TEXT("HomeLocation"));
	inline const FName PatrolLocation(TEXT("PatrolLocation"));
	inline const FName TargetFacingRange(TEXT("TargetFacingRange"));
	inline const FName CanChaseTarget(TEXT("CanChaseTarget"));
}

namespace SpEnemyAI::Define
{
	inline constexpr float ArriveRange(100.f);
	inline constexpr float FocusRange(300.f);
	inline constexpr float FacingToleranceDegrees(1.f);
}

// ==================================================

UCLASS()
class PROJECTSP_API ASpEnemyAIController : public ASpAIController
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAIPerceptionComponent> AIPerceptionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAISenseConfig_Sight> SightConfig;
	
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "MJ - Runtime", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpEnemyAIDefinition> EnemyAIDefinition;
	
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "MJ - Runtime", meta = (AllowPrivateAccess = "true"))
	FVector HomeLocation;
	
	// aggro
	TWeakObjectPtr<USpUnitAggroComponent> BoundAggroComponent;
	FDelegateHandle AggroChangedHandle;

	// ------------------------------------------------
	
	ASpEnemyAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	// virtual
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void OnUnitActive(bool bActive) override;

	// aggro
	void BindAggroComponent();
	void UnbindAggroComponent();
	void HandleAggroChanged();
	bool IsAggroTargetEligible(const class ASpUnit* Target) const;
	
public:
	virtual void SetDefinition(USpAIDefinition* Definition) override;
	
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
	
	UFUNCTION(BlueprintCallable)
	bool TryGetActivatableAbility(FGameplayTag& OutTag, bool& bNeedTarget, float& OutExecuteRange);
	
	// check
	bool CheckDistanceFromHome(const FVector& TargetLocation, float Distance) const;
	bool CheckNeedChase(const FVector& TargetLocation) const;
	bool CheckNeedReturn(FVector& OutLocation);
	
	// task
	bool TryGetRandomLocationInHomeRange(FVector& OutLocation);
	void RefreshCombatTarget();
};
