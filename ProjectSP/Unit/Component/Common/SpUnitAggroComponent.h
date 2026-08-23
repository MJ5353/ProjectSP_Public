#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpUnitAggroComponent.generated.h"

class UAggroDefinition;
class USpEnemyAIDefinition;
class USpUnitDefinition;
class ASpUnit;

// ==================================================

DECLARE_MULTICAST_DELEGATE(FSpAggroChanged);

// ==================================================

struct FSpAggroEntry
{
	// 누적 위협도
	float Threat = 0.0f;

	// 마지막으로 피해를 주거나 시야에 감지된 시각. 비가시 대상의 기억 만료 기준
	float LastStimulusTime = 0.0f;
	
	// true인 동안에는 AI가 현재 대상을 보고 있으므로 Threat를 감쇠하지 않음
	bool bVisible = false;
};

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitAggroComponent : public UActorComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "MJ - Runtime", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAggroDefinition> AggroDefinition;
	
	TMap<TWeakObjectPtr<ASpUnit>, FSpAggroEntry> ThreatEntries;
	FDelegateHandle DamageAppliedHandle;
	
public:
	FSpAggroChanged OnAggroChanged;
	
	USpUnitAggroComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// unit lifecycle
	virtual void OnInitUnit() override;
	virtual void OnClearUnit() override;
	virtual void OnUnitActive(bool bActive) override;

	// update threat
	void SetTargetSensed(AActor* Actor, bool bSensed); // 시야에 들어온 경우
	void AddDamageThreat(AActor* Instigator, float AppliedDamage); // 피해를 받은 경우
	void ClearThreat();

protected:
	// set
	void SetUp();
	
	// check target
	bool IsValidThreatTarget(const ASpUnit* TargetUnit) const;
	ASpUnit* ResolveThreatTarget(AActor* Instigator) const;

	// threat manage
	void UpdateThreatDecay(float DeltaTime);
	void BroadcastAggroChanged();

public:
	ASpUnit* GetBestTarget(AActor* CurrentTarget, float TargetSwitchRatio, TFunctionRef<bool(const ASpUnit*)> IsTargetEligible) const;
};
