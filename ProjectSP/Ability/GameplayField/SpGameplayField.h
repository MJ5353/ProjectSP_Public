#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayEffectTypes.h"
#include "ProjectSP/Ability/Targeting/SpTargetQueryTypes.h"
#include "SpGameplayFieldTypes.h"
#include "SpGameplayField.generated.h"

class ASpUnit;
class USceneComponent;
class UStaticMeshComponent;

// ==================================================

UCLASS(Abstract, Blueprintable)
class PROJECTSP_API ASpGameplayField : public AActor
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	FSpGameplayFieldConfig FieldConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	bool bAutoFitSingleVisualMesh = true;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_FieldActive, Category = "MJ - Replicate")
	bool bFieldActive = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_TargetQuery, Category = "MJ - Replicate")
	FSpTargetQuery TargetQuery;
	
	UPROPERTY(ReplicatedUsing = OnRep_VisualMeshRelativeZ)
	float VisualMeshRelativeZ = TNumericLimits<float>::Max(); // 0은 유효한 값!

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TObjectPtr<USceneComponent> SceneRoot;

private:
	bool bFollowingSource = false;
	uint32 SourceUnitUid = 0;
	FGenericTeamId SourceTeamId = FGenericTeamId::NoTeam;
	FDelegateHandle SourceUnavailableHandle;

	TArray<FGameplayEffectSpecHandle> EffectSpecs;
	TSet<TWeakObjectPtr<ASpUnit>> CurrentTargets;
	TMap<TWeakObjectPtr<ASpUnit>, TArray<FActiveGameplayEffectHandle>> AppliedEffectHandles;

	FVector LocalQueryOrigin = FVector::ZeroVector;
	FVector LocalQueryDirection = FVector::ForwardVector;
	FVector LocalQuerySegmentEnd = FVector::ZeroVector;

	FTimerHandle EvaluationTimerHandle;
	FTimerHandle LifetimeTimerHandle;

public:
	ASpGameplayField();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Gameplay Field")
	static ASpGameplayField* SpawnGameplayField(TSubclassOf<ASpGameplayField> FieldClass, ASpUnit* InSourceUnit, const FSpTargetQuery& InTargetQuery, const TArray<FGameplayEffectSpecHandle>& InEffectSpecs);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Gameplay Field")
	static ASpGameplayField* SpawnGameplayField_CustomConfig(TSubclassOf<ASpGameplayField> FieldClass, ASpUnit* InSourceUnit, const FSpGameplayFieldConfig& InFieldConfig, const FSpTargetQuery& InTargetQuery, const TArray<FGameplayEffectSpecHandle>& InEffectSpecs);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Gameplay Field")
	void EvaluateGameplayField();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Gameplay Field")
	void FinishGameplayField();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Gameplay Field")
	void DestroyGameplayField();

	UFUNCTION(BlueprintPure, Category="MJ - Gameplay Field")
	bool IsGameplayFieldActive() const { return bFieldActive; }

protected:
	virtual void ReleaseGameplayField();
	virtual bool ApplyFieldEffectsToTarget_Implementation(ASpUnit* Target);
	
	UFUNCTION(BlueprintNativeEvent, Category="MJ - Gameplay Field")
	bool ApplyFieldEffectsToTarget(ASpUnit* Target);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="MJ - Gameplay Field", meta=(DisplayName="On Gameplay Field Activated"))
	void K2_OnGameplayFieldActivated();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category="MJ - Gameplay Field", meta=(DisplayName="On Gameplay Field Finished"))
	void K2_OnGameplayFieldFinished();

	UFUNCTION(BlueprintImplementableEvent, Category="MJ - Gameplay Field", meta=(DisplayName="On Target Entered Gameplay Field"))
	void K2_OnTargetEntered(ASpUnit* Target);

	UFUNCTION(BlueprintImplementableEvent, Category="MJ - Gameplay Field", meta=(DisplayName="On Target Exited Gameplay Field"))
	void K2_OnTargetExited(ASpUnit* Target);

private:
	// static
	static bool CanUseSourceForSpawn(const ASpUnit* InSourceUnit);
	static bool IsSpawnedFieldAvailable(const ASpGameplayField* Field);
	static void RemoveEffectHandlesFromTarget(ASpUnit* Target, const TArray<FActiveGameplayEffectHandle>& Handles);
	
	// init
	bool InitializeGameplayField(ASpUnit* InSourceUnit, const FSpTargetQuery& InTargetQuery, const TArray<FGameplayEffectSpecHandle>& InEffectSpecs);
	void SetFieldStep1_TargetQuery(const FSpTargetQuery& InTargetQuery);
	void SetFieldStep2_Source(ASpUnit* InSourceUnit);
	void SetFieldStep3_FollowingSource(ASpUnit* InSourceUnit);
	void SetFieldStep4_EffectSpec(const TArray<FGameplayEffectSpecHandle>& InEffectSpecs);
	void StartFieldTimers();
	void SetVisualMeshGroundOffset(const ASpUnit* InSourceUnit, const FVector& InOrigin);
	void ApplyVisualMeshRelativeZ();

	// refresh
	void RefreshField();
	void RefreshBoundTargetQuery();
	void AutoFitVisualMeshToTargetQuery();
	
	// target
	void ProcessCollectedTargets(const TArray<ASpUnit*>& CollectedTargets);
	void ProcessCollectedTarget(ASpUnit* Target);
	void NotifyExitedTargets(const TSet<TWeakObjectPtr<ASpUnit>>& RemainingTargets);
	void HandleTargetEntered(ASpUnit* Target);
	void HandleTargetExited(ASpUnit* Target);
	void HandleSourceUnavailable();

	// remove
	void ReleaseFailedGameplayField();
	void RemoveEffectsOnFinishIfConfigured();
	void RemoveTrackedEffectsFromTarget(ASpUnit* Target);
	void RemoveAllTrackedEffects();
	void ClearFieldTimers();
	void UnbindSourceUnavailable();
	
	// helper
	bool CheckCanContinueField() const;
	bool ShouldApplyEffectsToTarget(bool bNewTarget) const;
	bool GetVisualQueryDimensions(float& OutLength, float& OutWidth, float& OutCenterOffsetX) const;

	// on rep ------------------------------------------------
	
	UFUNCTION()
	void OnRep_TargetQuery();

	UFUNCTION()
	void OnRep_FieldActive();

	UFUNCTION()
	void OnRep_VisualMeshRelativeZ();
};
