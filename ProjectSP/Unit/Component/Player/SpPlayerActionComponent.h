#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "InputTriggers.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpPlayerActionComponent.generated.h"

class ASpUnit;
class ASpPlayerUnit;
class AActor;
class USpInputDefinition;
class USpInputBehavior;
class USpPlayerInputComponent;
struct FSpInputActionBinding;
enum class ESpInputBehaviorRequest : uint8;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpPlayerActionComponent : public UPawnComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TMap<FGameplayTag, TObjectPtr<USpInputBehavior>> InputBehaviors;

	UPROPERTY(Transient)
	TObjectPtr<USpInputBehavior> ActiveBehavior;

	// ------------------------------------------------

	TWeakObjectPtr<USpPlayerInputComponent> BoundInputComponent;
	FDelegateHandle InputEventHandle;

	FGameplayTagContainer HeldInputTags;
	TMap<FGameplayTag, FInputActionValue> HeldInputValues;

	FGameplayTag ActiveBehaviorInputTag;
	FGameplayTag InputTagToReinterpret;
	FGameplayTag AbilityTagBlockingReinterpret;

public:
	USpPlayerActionComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// unit manage
	virtual void OnClearUnit() override;
	virtual void OnUnitActive(bool bActive) override;
	virtual void OnUnitPlayable(bool bPlayable) override;

	// active behavior
	void SetInputBehavior(FGameplayTag SourceInputTag, USpInputBehavior& Behavior);
	void ClearInputBehaviorState();

private:
	// handle input
	void HandleTaggedInput(FGameplayTag InputTag, const FInputActionValue& Value, ETriggerEvent TriggerEvent);
	bool ApplyHeldTag(FGameplayTag InputTag, ETriggerEvent TriggerEvent);
	void ApplyBehaviorResult(USpInputBehavior& Behavior, FGameplayTag SourceInputTag, ESpInputBehaviorRequest StateChange);

	// process behavior
	void ProcessDeferredHeldInput(ASpPlayerUnit& Unit);
	void TickActiveBehavior(ASpPlayerUnit& Unit, float DeltaTime);

	// clear
	void ClearReinterpretState();
	void ClearInputState();

	// target
	UFUNCTION(Server, Reliable)
	void ServerSetTargetActor(ASpUnit* InTargetActor, bool bFaceTarget);

public:
	bool IsInputHeld(FGameplayTag InputTag) const;
	void SyncTargetToServer(bool bFaceTarget = false);

	UFUNCTION(BlueprintPure)
	ASpUnit* GetTargetActor() const;

private:
	// get
	const USpInputDefinition* GetInputDefinition() const;
	USpInputBehavior* GetInputBehavior(const FSpInputActionBinding& Binding);
};
