#pragma once

#include "AbilitySystemInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpPlayerState.generated.h"

class ASpPlayerUnit;
class ASpPlayerState;
class UAbilitySystemComponent;
class USpAbilitySystemComponent;
class USpInputDefinition;
class USpCameraDefinition;
class USpUnitDefinition;

// ==================================================

// 클라/서버가 공유하는 판 상태

UCLASS()
class PROJECTSP_API ASpPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<USpUnitDefinition> UnitDefinition;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReadyToStart, Category = "MJ | Replicate")
	bool bReadyToStart = false;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ | Component")
	TObjectPtr<USpAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TObjectPtr<USpUnitDefinition> GrantedAbilityUnitDefinition;

	// ------------------------------------------------

public:
	ASpPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void InitializeAbilitySystem(ASpPlayerUnit* InAvatar);
	void SetReadyToStart_Server(bool bInReadyToStart);
	
	// get
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	USpAbilitySystemComponent* GetSpAbilitySystemComponent() const;

protected:
	void NotifyLobbyStateChanged();

	UFUNCTION()
	void OnRep_ReadyToStart();
};
