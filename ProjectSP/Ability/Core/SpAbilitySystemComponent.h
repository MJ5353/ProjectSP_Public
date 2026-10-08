#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "ProjectSP/Ability/Core/SpGrantedAbilityHandles.h"
#include "ProjectSP/Ability/Core/SpGrantedAttributeSets.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpAbilitySystemComponent.generated.h"

struct FSpAttributeInitValue;
class USpUnitDefinition;
class USpAbilityExtensionComponent;
class USpAbilityExtensionDefinition;

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
	
	// friend
	friend class USpAbilityExtensionComponent;

public:
	FSpDamageApplied OnDamageApplied;
	FSpReplicatedGameplayEventDelegate OnReplicatedGameplayEvent;

protected:
	UPROPERTY(Transient)
	FSpGrantedAbilityHandles GrantedAbilityHandles;

	UPROPERTY(Transient)
	FSpGrantedAttributeSets GrantedAttributeSets;

	UPROPERTY(Transient, BlueprintReadOnly, Replicated, Category = "MJ - Replicate")
	FGameplayTagContainer EquippedAbilityExtensions;

	UPROPERTY(Transient, BlueprintReadOnly, Replicated, Category = "MJ - Replicate")
	TObjectPtr<USpAbilityExtensionDefinition> ExtensionDefinition;

	bool bOnSpawnAbilitiesActivated = false;
	bool bUnitSetupComplete = false;

	// core ------------------------------------------------
	
	void SetUp(const USpUnitDefinition* UnitDefinition);
	void SetUpAbilityAndAttribute(const USpUnitDefinition& UnitDefinition);
	void SetUpAbilityExtension(USpAbilityExtensionDefinition* InDefinition, const FGameplayTagContainer& Extensions);
	void Clear();
	void TryActivateOnSpawnAbilities();

public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	
	// reset
	void ResetForDead();
	void ResetForPool();
	
	// unit
	virtual void OnUnitActive(bool bActive) override;
	bool HasCompletedUnitSetup() const { return bUnitSetupComplete; }

	// attribute ------------------------------------------------
	
	template <typename UserClass>
	FDelegateHandle RegisterAttributeChangeCallback(const FGameplayAttribute& Attribute, UserClass* Object, void (UserClass::*Delegate)(const FOnAttributeChangeData&));
	
	void UnregisterAttributeChangeCallback(const FGameplayAttribute& Attribute, FDelegateHandle& Handle);
	void NotifyDamageApplied(AActor* Instigator, float AppliedDamage);
	void SendHitEventToTarget(AActor* Instigator);
	void RestoreHp();

	// ability tag ------------------------------------------------
	
	bool IsAbilityTagActive(FGameplayTag Tag) const;
	const FGameplayAbilitySpec* FindAbilitySpecByTag(const FGameplayTag& Tag) const;

	UFUNCTION(BlueprintCallable)
	bool TryActivateAbilityByTag(FGameplayTag Tag);

	UFUNCTION(BlueprintPure)
	bool CanActivateAbilityByTag(FGameplayTag Tag) const;

	UFUNCTION(BlueprintPure)
	bool HasAbilityAssetTag(FGameplayTag AbilityTag, FGameplayTag AssetTag) const;

	// extension ------------------------------------------------

	bool CanEquipAbilityExtension(FGameplayTag AbilityKey, FGameplayTag ExtensionTag) const;
	bool IsAbilityExtensionTag(FGameplayTag AbilityKey, FGameplayTag ExtensionTag) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Ability")
	bool AcquireAbilityExtension(FGameplayTag AbilityKey, FGameplayTag ExtensionTag);

	UFUNCTION(BlueprintPure, Category="MJ - Ability")
	bool GetAbilityCooldownByTag(FGameplayTag Tag, float& OutRemaining, float& OutDuration) const;

	UFUNCTION(BlueprintPure, Category="MJ - Ability")
	bool HasEquippedAbilityExtension(FGameplayTag AbilityKey, FGameplayTag ExtensionTag) const;

	UFUNCTION(BlueprintPure, Category="MJ - Ability")
	bool TryGetAbilityExecuteRangeByTag(FGameplayTag AbilityTag, float& OutExecuteRange) const;

	UFUNCTION(BlueprintPure, Category="MJ - Ability")
	FGameplayTagContainer GetEquippedAbilityExtensions() const { return EquippedAbilityExtensions; }
	
	FGameplayTag FindAbilityKeyByExtension(FGameplayTag ExtensionTag) const;

	// gameplay event ------------------------------------------------
	
	void SendReplicatedGameplayEvent_Client(const FGameplayAbilitySpecHandle& AbilityHandle, const FPredictionKey& ActivationPredictionKey, const FGameplayEventData& Payload);
	
protected:
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
