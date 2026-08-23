#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectSP/Ability/SpGrantedAbilityHandles.h"
#include "ProjectSP/Ability/SpGrantedAttributeSets.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandTypes.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "TimerManager.h"
#include "SpAbilitySystemComponent.generated.h"

struct FSpAttributeInitValue;
class USpUnitDefinition;
class UAnimMontage;
class UGameplayAbility;

// ==================================================

// damage
DECLARE_MULTICAST_DELEGATE_TwoParams(FSpDamageApplied, AActor* /* Instigator */, float /* AppliedDamage */);

// gameplay event
DECLARE_MULTICAST_DELEGATE_ThreeParams(FSpReplicatedGameplayEventDelegate,
	const FGameplayAbilitySpecHandle& /* AbilityHandle */,
	const FPredictionKey& /* ActivationPredictionKey */,
	const FGameplayEventData& /* Payload */
);

// ------------------------------------------------

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpAbilitySystemComponent : public UAbilitySystemComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

public:
	FSpDamageApplied OnDamageApplied;
	FSpReplicatedGameplayEventDelegate OnReplicatedGameplayEvent;

protected:
	UPROPERTY(Transient)
	FSpGrantedAbilityHandles GrantedAbilityHandles;

	UPROPERTY(Transient)
	FSpGrantedAttributeSets GrantedAttributeSets;

	TArray<FGameplayAbilitySpecHandle> InputPressedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputReleasedSpecHandles;
	TArray<FGameplayAbilitySpecHandle> InputHeldSpecHandles;
	TArray<FGameplayAbilitySpecHandle> AbilitiesToActivate;

	bool bOnSpawnAbilitiesActivated = false;
	bool bPlayerStateMontageReplicationBoostActive = false;
	float SavedPlayerStateNetUpdateFrequency = 0.0f;
	TWeakObjectPtr<AActor> MontageReplicationPlayerState;
	FTimerHandle MontageReplicationRestoreTimer;

	// ------------------------------------------------
	
	// Init
	void SetUpAbilityAndAttribute(const USpUnitDefinition& UnitDefinition);
	void TryActivateOnSpawnAbilities();
	void BeginPlayerStateMontageReplicationBoost();
	void SchedulePlayerStateMontageReplicationRestore();
	void RestorePlayerStateMontageReplicationRate();
	void ClearPlayerStateMontageReplicationRestoreTimer();
	
	// clear
	void Clear();
	void ClearAbilityInput();

	// ability spec lookup
	const FGameplayAbilitySpec* FindAbilitySpecByTag(const FGameplayTag& Tag) const;
	
public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	virtual void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec) override;
	virtual void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec) override;
	virtual float PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None, float StartTimeSeconds = 0.0f) override;
	virtual void ClearAnimatingAbility(UGameplayAbility* Ability) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// state
	virtual void OnUnitActive(bool bActive) override;
	
	// Set
	void SetUp(const USpUnitDefinition* UnitDefinition);
	void ResetForPool();
	
	// attribute
	template <typename UserClass>
	FDelegateHandle RegisterAttributeChangeCallback(const FGameplayAttribute& Attribute, UserClass* Object, void (UserClass::*Delegate)(const FOnAttributeChangeData&));
	
	void UnregisterAttributeChangeCallback(const FGameplayAttribute& Attribute, FDelegateHandle& Handle);
	void RestoreHp();
	void NotifyDamageApplied(AActor* Instigator, float AppliedDamage);
	
	// ability input
	void OnAbilityInputTagPressed(FGameplayTag InputTag);
	void OnAbilityInputTagReleased(FGameplayTag InputTag);
	void OnProcessAbilityInput(float DeltaTime, bool bGamePaused); // 누르고 있는 동안
	bool IsAbilityTagActive(FGameplayTag Tag) const;
	
	// ability tag
	UFUNCTION(BlueprintCallable)
	bool TryActivateAbilityByTag(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable)
	bool CanActivateAbilityByTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintCallable)
	bool HasAbilityAssetTag(FGameplayTag AbilityTag, FGameplayTag AssetTag) const;

	UFUNCTION(BlueprintCallable)
	bool GetAbilityCooldownByTag(FGameplayTag Tag, float& OutRemaining, float& OutDuration) const;
		
	// player command
	void SubmitPlayerCommand_Client(const FSpPlayerCommandInput& CommandInput);
	bool TryActivateAbilityForPlayerCommand_Client(FGameplayTag AbilityTag, uint16 CommandId);
	
	// gameplay event
	void SendReplicatedGameplayEvent_Client(const FGameplayAbilitySpecHandle& AbilityHandle, const FPredictionKey& ActivationPredictionKey, const FGameplayEventData& Payload);
	
protected:
	UFUNCTION(Server, Reliable)
	void ServerSubmitPlayerCommand(FSpPlayerCommandInput CommandInput);

	UFUNCTION(Server, Unreliable)
	void ServerUpdatePlayerCommand(FSpPlayerCommandInput CommandInput);

	UFUNCTION(Server, Reliable)
	void ServerConfirmPredictedAbilityForCommand(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey ActivationPredictionKey, FGameplayTag AbilityTag, uint16 CommandId);

	UFUNCTION(Server, Reliable)
	void ServerSendReplicatedGameplayEvent(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey ActivationPredictionKey, FGameplayEventData Payload, FPredictionKey PredictionKey);
};

// ==================================================

template <typename UserClass>
FDelegateHandle USpAbilitySystemComponent::RegisterAttributeChangeCallback(const FGameplayAttribute& Attribute, UserClass* Object, void(UserClass::* Delegate)(const FOnAttributeChangeData&))
{
	if (!Attribute.IsValid() || !Object || Delegate == nullptr)
		return FDelegateHandle();

	FOnGameplayAttributeValueChange& ChangeDelegate = GetGameplayAttributeValueChangeDelegate(Attribute);
	return ChangeDelegate.AddUObject(Object, Delegate);
}
