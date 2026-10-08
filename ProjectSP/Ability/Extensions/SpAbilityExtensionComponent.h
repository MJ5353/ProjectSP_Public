#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "SpAbilityExtensionComponent.generated.h"

class ASpPlayerUnit;
class USpAbilitySystemComponent;
class USpAbilityExtensionDefinition;
class USpUnitDefinition;

// ==================================================

USTRUCT()
struct FSpAbilityExtensionActiveState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USpUnitDefinition> UnitDefinition;

	UPROPERTY()
	TObjectPtr<USpAbilityExtensionDefinition> ExtensionDefinition;

	UPROPERTY()
	FGameplayTagContainer EquippedExtensions;
};

UENUM(BlueprintType)
enum class ESpAbilityExtensionPurchaseResult : uint8
{
	Success,
	NotPlaying,
	NoActiveUnit,
	InvalidExtension,
	AlreadyUnlocked,
	InsufficientPoints,
	CannotEquip,
};

// ------------------------------------------------

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSpExtensionPointsChanged, int32, NewPoints);

// ------------------------------------------------

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpAbilityExtensionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, Category="MJ - Ability")
	TObjectPtr<USpAbilityExtensionDefinition> Definition; // 조종중인 Unit의 Extension Definition

	UPROPERTY(Transient, BlueprintReadOnly, Category="MJ - Ability")
	FGameplayTagContainer EquippedExtensions;

	UPROPERTY(BlueprintAssignable, Category="MJ - Ability")
	FSpExtensionPointsChanged OnExtensionPointsChanged;

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<USpUnitDefinition>, FGameplayTagContainer> UnlockedExtensionsByUnit;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_ActiveState)
	FSpAbilityExtensionActiveState ActiveState;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_ExtensionPoints)
	int32 ExtensionPoints = 0;

public:
	// -------------------------------------------------------

	USpAbilityExtensionComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// unit sync ---------------------------------------------

	void SetUp_Server(const USpUnitDefinition* UnitDefinition, bool bPawnChanged);

	// extension change ---------------------------------------

	ESpAbilityExtensionPurchaseResult TryPurchaseExtension_Server(FGameplayTag ExtensionTag);
	void GrantExtensionPoint_Server();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Ability|Extension")
	bool AcquireExtension_Server(FGameplayTag AbilityKey, FGameplayTag ExtensionTag);

	UFUNCTION(BlueprintImplementableEvent, Category="MJ - Ability|Extension")
	void K2_OnExtensionStateChanged();

private:
	bool AddEquippedExtension_Server(FGameplayTag AbilityKey, FGameplayTag ExtensionTag);
	void ApplyToASC_Server();
	void RefreshPublishedState();
	void NotifyStateChanged_Server();

	// rpc -----------------------------------------------------

	UFUNCTION()
	void OnRep_ActiveState();

	UFUNCTION()
	void OnRep_ExtensionPoints();

public:
	// get -----------------------------------------------------

	UFUNCTION(BlueprintPure, Category="MJ - Ability|Extension")
	bool HasEquippedExtension(FGameplayTag ExtensionTag) const { return EquippedExtensions.HasTagExact(ExtensionTag); }

	UFUNCTION(BlueprintPure, Category="MJ - Ability|Extension")
	int32 GetExtensionPoints() const { return ExtensionPoints; }

private:
	bool IsConfigured(FGameplayTag AbilityKey, FGameplayTag ExtensionTag) const;
	USpAbilitySystemComponent* GetAbilitySystemComponent() const;
};
