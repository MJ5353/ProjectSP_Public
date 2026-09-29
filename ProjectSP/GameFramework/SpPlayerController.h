#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectSP/Ability/Extensions/SpAbilityExtensionComponent.h"
#include "SpPlayerController.generated.h"

class ASpPlayerState;
class ASpPlayerUnit;
class USpUnitDefinition;
class UInputMappingContext;

// ==================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSpAbilityExtensionPurchaseResolved, FGameplayTag, ExtensionTag, ESpAbilityExtensionPurchaseResult, Result);

// ==================================================

UCLASS()
class PROJECTSP_API ASpPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MJ - Ability")
	TObjectPtr<USpAbilityExtensionComponent> AbilityExtensionComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<const UInputMappingContext>> ActiveUnitInputMappings;

public:
	UPROPERTY(BlueprintAssignable, Category="MJ - Ability|Extension")
	FSpAbilityExtensionPurchaseResolved OnAbilityExtensionPurchaseResolved;

	ASpPlayerController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// server ------------------------------------------------

	void SyncAbilityExtensions_Server(bool bPawnChanged);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetReady(bool bReadyToStart);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerRequestStart();

	UFUNCTION(Server, Reliable)
	void ServerReportInitialPresentationReady(uint32 InitialPresentationId);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MJ - Ability|Extension")
	void ServerPurchaseAbilityExtension(FGameplayTag ExtensionTag);

	UFUNCTION(Client, Reliable)
	void ClientAbilityExtensionPurchaseResult(FGameplayTag ExtensionTag, ESpAbilityExtensionPurchaseResult Result);

	UFUNCTION(Server, Reliable, BlueprintCallable, Category="MJ - Unit")
	void ServerSelectPlayableUnit(USpUnitDefinition* UnitDefinition);

	// get ------------------------------------------------

	bool IsGameplayInputEnabled_Client() const;
	bool ApplyUnitInputMappings_Client(const USpUnitDefinition* UnitDefinition);

	UFUNCTION(BlueprintPure, Category="MJ - Ability")
	USpAbilityExtensionComponent* GetAbilityExtensionComponent() const { return AbilityExtensionComponent; }
};
