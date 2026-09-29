#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "SpGameplayAbility.generated.h"

class ASpUnit;
class USpAbilityModifierBase;

// ==================================================

UENUM(BlueprintType)
enum class EMjAbilityActivationPolicy : uint8
{
	Manual,
	OnSpawn,
};

UENUM()
enum class ESpAbilityTagBranchResult : uint8
{
	Success,
	Failure, 
};

// ==================================================

UCLASS()
class PROJECTSP_API USpGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting")
	EMjAbilityActivationPolicy ActivationPolicy = EMjAbilityActivationPolicy::Manual;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Setting", meta=(ClampMin="0.0"))
	float ExecuteRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="MJ - Cooldown", meta=(ClampMin="0.0"))
	float CooldownDuration = 0.0f;

protected:
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "MJ - Setting")
	TArray<TObjectPtr<USpAbilityModifierBase>> Modifiers;

	TWeakObjectPtr<ASpUnit> ActivationTarget;

	bool bModifierLifecycleActive = false;
	bool bDispatchingModifierEnd = false;

	// ------------------------------------------------
	
public:
	using UGameplayAbility::GetCooldownTimeRemaining;

	virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CommitAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) override;

protected:
	virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category="MJ - Ability", meta=(ExpandEnumAsExecs="ReturnValue"))
	ESpAbilityTagBranchResult HasTagBranch(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Ability")
	void ApplyAttackDamageToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> DamageEffectClass);

protected:
	void SendHitReactionEvent(AActor* TargetActor, const FGameplayEffectSpecHandle& DamageSpec);

public:
	// cooldown
	virtual void ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual bool CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const override;
	virtual void GetCooldownTimeRemainingAndDuration(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& OutCooldownDuration) const override;
	virtual UGameplayEffect* GetCooldownGameplayEffect() const override;

protected:
	FGameplayTag GetCooldownAbilityKey(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
	
public:
	UFUNCTION(BlueprintPure, Category="MJ - Ability")
	ASpUnit* GetActivationTargetActor() const { return ActivationTarget.Get(); }
};
