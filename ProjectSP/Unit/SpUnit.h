#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "ModularCharacter.h"
#include "AbilitySystemInterface.h"
#include "Components/ActorComponent.h"
#include "GenericTeamAgentInterface.h"
#include "GameplayTagAssetInterface.h"
#include "Define/SpUnitData.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "SpUnit.generated.h"

class USpUnitStimuliSourceComponent;
class USpUnitStateComponent;
class USpUnitHPBarComponent;
class USpAbilitySystemComponent;
class USpSkillDisplayDefinition;
class USpUnitClientGatewayComponent;
class USpUnitServerGatewayComponent;
class UActorComponent;

// ==================================================

DECLARE_MULTICAST_DELEGATE(FSpUnitTargetChanged);
DECLARE_MULTICAST_DELEGATE(FSpUnitTargetInvalidated);
DECLARE_MULTICAST_DELEGATE(FSpUnitSourceUnavailable);

// ------------------------------------------------

UCLASS()
class PROJECTSP_API ASpUnit : public AModularCharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface, 
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
protected:	
	friend class USpUnitServerGatewayComponent;
	friend class USpUnitClientGatewayComponent;
	friend class UUnitRegistrySubsystem;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitStateComponent> UnitStateComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitServerGatewayComponent> ServerGateway;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitClientGatewayComponent> ClientGateway;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_UnitData, Category = "MJ - Replicate")
	FSpUnitData UnitData;

	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = "MJ - Runtime", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<UActorComponent>> UnitManageComponents;
	
	// init
	bool bUnitManageComponentsInitialized = false;
	
	// tag
	TWeakObjectPtr<USpAbilitySystemComponent> UnitTagChangedEventASC;
	FGameplayTagContainer AppliedUnitStateTags; // noti한 state tag
	FDelegateHandle UnitTagChangedEventHandle;
	
	// dead presentation
	bool bDeadPresentationActive = false;
	FTimerHandle DeadProcessTimerHandle;
	
	// target
	TWeakObjectPtr<ASpUnit> TargetActor;
	FSpUnitTargetInvalidated OnTargetInvalidated;
	FDelegateHandle TargetInvalidatedHandle;
	
public:
	FSpUnitTargetChanged OnTargetChanged;
	FSpUnitSourceUnavailable OnSourceUnavailable;

protected:
	ASpUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Core ------------------------------------------------
	void InitUnit();
	void ClearUnit();
	void SetUnitPlayable(bool bPlayable);
	void ResetBattleFlag();
	
public:
	void SetUnitActive_Server(bool bActive, bool bSetPlayable);
	void SetUnitPresentationVisible_Client(bool bVisible);
	void SetUnitPresentationVisible_Server(bool bVisible);
	
protected:
	// Comp
	void CacheUnitManageComponents();
	void InitUnitManageComponents();
	void ClearUnitManageComponents();

	// Unit Data 
	void ApplyUnitData_Server(const FSpUnitData& InUnitData);
	void ResetUnitData_Server();
	void ApplyReplicatedUnitData_Client();

	UFUNCTION()
	virtual void OnRep_UnitData();

public:
	// Dead ------------------------------------------------
	void Dead();

	UFUNCTION(BlueprintImplementableEvent, Category="MJ - Unit")
	void K2_BeginDeadPresentation();

	UFUNCTION(BlueprintImplementableEvent, Category="MJ - Unit")
	void K2_FinishDeadPresentation();

protected:
	void BeginDead();
	void StartDeadProcess_Server();
	void FinishDeadProcess_Server();
	void StartDeadPresentation_Client();
	void FinishDeadPresentation_Client();
	virtual void HandleDeadProcessFinished_Server();
	
public:
	// Tag ------------------------------------------------
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	void AddLocalTag(FGameplayTag Tag);
	void RemoveLocalTag(FGameplayTag Tag);
	void AddReplicatedTag(FGameplayTag Tag);
	void RemoveReplicatedTag(FGameplayTag Tag);
	bool HasTag(FGameplayTag Tag, bool bExact) const;

protected:
	// tag event
	void RegisterUnitTagChangedEvent();
	void UnregisterUnitTagChangedEvent();
	void SyncUnitStatesFromAbilitySystem(USpAbilitySystemComponent* ASC);
	void ResetAppliedUnitStates();
	void HandleUnitTagChanged(const FGameplayTag Tag, int32 NewCount);
	
public:
	// Target ---------------------------------------------
	void SetTargetActor(ASpUnit* InTargetActor);
	
protected:
	void InvalidateTarget();
	void HandleTargetInvalidated();
	
public:
	 // Movement ------------------------------------------
	bool MoveToTargetLocation(const FVector& TargetLocation, float MoveEndDistance, float SpeedScale);
	bool RotateToTargetLocation(const FVector& TargetLocation, float DeltaTime, float SpeedScale);
	bool IsFacingTargetLocation(const FVector& TargetLocation) const;
	void ApplyRotationRateFromAttribute();

protected:
	// State ----------------------------------------------
	void NotifyUnitStateChanged(FGameplayTag StateTag, bool bAdded);
	void HandleStateChanged(FGameplayTag StateTag, bool bAdded);
	void OnDead();
	
public:
	// Get ------------------------------------------------
	template <typename TComponent>
	TComponent* GetUnitComponent() const;
	
	bool CheckDead() const;
	bool IsAttackable(const ASpUnit* Target) const;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// team
	UFUNCTION(BlueprintPure)
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	ETeamAttitude::Type GetTeamAttitudeTowards(const IGenericTeamAgentInterface* OtherTeamAgent) const;

	const FSpUnitData& GetUnitData() const { return UnitData; }
	bool HasValidUnitData() const { return UnitData.IsValid(); }
	uint32 GetUnitUid() const { return UnitData.UnitUid; }
	USpUnitServerGatewayComponent* GetServerGateway() const { return ServerGateway; }
	USpUnitClientGatewayComponent* GetClientGateway() const { return ClientGateway; }
	
	UFUNCTION(BlueprintPure)
	ASpUnit* GetTargetActor() const { return TargetActor.Get(); }

	UFUNCTION(BlueprintPure)
	const USpUnitDefinition* GetUnitDefinition() const { return UnitData.UnitDefinition; }
	
	UFUNCTION(BlueprintPure)
	virtual USpAbilitySystemComponent* GetSpAbilitySystemComponent() const { return AbilitySystemComponent; }

	UFUNCTION(BlueprintPure, Category="MJ - Skill")
	USpSkillDisplayDefinition* GetSkillDisplayDefinition() const { return UnitData.UnitDefinition ? UnitData.UnitDefinition->SkillDisplayDefinition.Get() : nullptr; }
};

// ==================================================

template <typename TComponent>
TComponent* ASpUnit::GetUnitComponent() const
{
	static_assert(TIsDerivedFrom<TComponent, UActorComponent>::IsDerived, "TComponent must derive from UActorComponent");
	return FindComponentByClass<TComponent>();
}
