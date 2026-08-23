#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "SpAbilityTask_WaitReplicatedGameplayEvent.generated.h"

// ==================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpWaitReplicatedGameplayEventDelegate, FGameplayEventData, Payload);

// ------------------------------------------------

UCLASS()
class PROJECTSP_API USpAbilityTask_WaitReplicatedGameplayEvent : public UAbilityTask
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FSpWaitReplicatedGameplayEventDelegate EventReceived;

private:
	FGameplayTag EventTag;
	bool bOnlyTriggerOnce = false;
	FDelegateHandle LocalGameplayEventHandle;
	FDelegateHandle ReplicatedGameplayEventHandle;
	
protected:
	USpAbilitySystemComponent* GetSpAbilitySystemComponent() const;
	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityEnding) override;

private:
	// handle
	void HandleLocalGameplayEvent(const FGameplayEventData* Payload);
	void HandleReplicatedGameplayEvent(const FGameplayAbilitySpecHandle& AbilityHandle, const FPredictionKey& ActivationPredictionKey, const FGameplayEventData& Payload);
	void BroadcastEvent(FGameplayEventData Payload);
	
public:
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static USpAbilityTask_WaitReplicatedGameplayEvent* WaitReplicatedGameplayEvent(
		UGameplayAbility* OwningAbility, 
		UPARAM(meta = (GameplayTagFilter = "GameplayEventTagsCategory")) 
		FGameplayTag EventTag, 
		bool bOnlyTriggerOnce = false);
};
