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
#include "ProjectSP/Subsystem/ActorPool/SpPoolableActor.h"
#include "SpUnit.generated.h"

class USpUnitStimuliSourceComponent;
class USpUnitStateComponent;
class USpUnitHPBarComponent;
class USpAbilitySystemComponent;
class USpUnitClientGatewayComponent;
class USpUnitServerGatewayComponent;
class UActorComponent;

// ==================================================

UCLASS()
class PROJECTSP_API ASpUnit : public AModularCharacter, public IAbilitySystemInterface, public IGameplayTagAssetInterface, 
	public IGenericTeamAgentInterface, public ISpPoolableActor
{
	GENERATED_BODY()
	
protected:
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
	
	FDelegateHandle UnitTagChangedEventHandle;
	FTimerHandle DeadProcessTimerHandle;
	bool bDeadPresentationStarted = false;
	
public:
	ASpUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void BeginPlay() override;
	
	// pool
	virtual void Push() override;
	virtual void OnCreate() override;
	virtual void OnDestroy() override;
	virtual void OnSpawn() override;
	virtual void OnReturn() override;
	
	// life cycle
	void InitUnit();
	void ClearUnit();
	
	// get
	template <typename TComponent>
	TComponent* GetUnitComponent() const;

	const FSpUnitData& GetUnitData() const { return UnitData; }
	const USpUnitDefinition* GetUnitDefinition() const { return UnitData.UnitDefinition; }
	uint32 GetUnitUid() const { return UnitData.UnitUid; }
	bool HasValidUnitData() const { return UnitData.IsValid(); }
	USpUnitServerGatewayComponent* GetServerGateway() const { return ServerGateway; }
	USpUnitClientGatewayComponent* GetClientGateway() const { return ClientGateway; }

	// 상태
	void Dead();

	UFUNCTION(BlueprintCallable, Category="MJ - Unit")
	void K2_BeginDeadPresentation();

	UFUNCTION(BlueprintCallable, Category="MJ - Unit")
	void K2_FinishDeadPresentation();
	
	// 팀
	UFUNCTION(BlueprintCallable)
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	ETeamAttitude::Type GetTeamAttitudeTowards(const IGenericTeamAgentInterface* OtherTeamAgent) const;
	
	// 태그
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;
	void AddLocalTag(FGameplayTag Tag);
	void RemoveLocalTag(FGameplayTag Tag);
	void AddReplicatedTag(FGameplayTag Tag);
	void RemoveReplicatedTag(FGameplayTag Tag);
	bool HasTag(FGameplayTag Tag, bool bExact = false) const;
	
	// check
	bool IsTargetable(const ASpUnit* Attacker) const;
	bool IsAttackable(const ASpUnit* Attacker) const;
	
	// 이동
	bool MoveToTargetLocation(const FVector& TargetLocation, float MoveEndDistance, float SpeedScale);
	bool RotateToTargetLocation(const FVector& TargetLocation, float DeltaTime, float SpeedScale);
	void ApplyRotationRateFromAttribute();
	
	// 어빌리티
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintCallable)
	virtual USpAbilitySystemComponent* GetSpAbilitySystemComponent() const;

	// 복제
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	friend class USpUnitServerGatewayComponent;
	friend class USpUnitClientGatewayComponent;

	// 서버 전용 유닛 생명주기
	void ApplyUnitData_ServerOnly(const FSpUnitData& InUnitData);
	void ResetUnitData_ServerOnly();
	void SetUnitActive_ServerOnly(bool bActive);

	// 클라이언트 전용 복제 데이터 연결 지점
	void ApplyReplicatedUnitData_ClientOnly();
	
	// battle flag tag
	void ResetBattleFlag();

	// 컴포넌트 설정
	void CacheUnitManageComponents();
	void InitUnitManageComponents();
	void ClearUnitManageComponents();
	
	// 태그 이벤트
	void RegisterUnitTagChangedEvent();
	void UnregisterUnitTagChangedEvent();
	void HandleUnitTagChanged(const FGameplayTag Tag, int32 NewCount);
	
	// 사망
	void BeginDeadPresentation();
	void FinishDeadPresentation();
	void StartDeadProcess();
	void FinishDeadProcess();
	void DestroyByDeadProcess();
	void ResurrectByDeadProcess();

	// 상태
	void NotifyUnitStateChanged(FGameplayTag StateTag, bool bAdded);
	void HandleStateChanged(FGameplayTag StateTag, bool bAdded);

	// 복제
	UFUNCTION()
	void OnRep_UnitData();
};

// ==================================================

template <typename TComponent>
TComponent* ASpUnit::GetUnitComponent() const
{
	static_assert(TIsDerivedFrom<TComponent, UActorComponent>::IsDerived, "TComponent must derive from UActorComponent");
	return FindComponentByClass<TComponent>();
}
