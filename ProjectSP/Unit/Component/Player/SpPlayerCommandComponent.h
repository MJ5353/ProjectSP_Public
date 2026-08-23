#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandService_Movement.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandResolver.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandRuntime.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpPlayerCommandComponent.generated.h"

class ASpUnit;
struct FGameplayTag;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpPlayerCommandComponent : public UPawnComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	float BasicAttackRange = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	float AttackMoveRepathDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	float MoveRepathDistance = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	float MoveRepathInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	float MoveEndDistance = 50.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	float CursorTraceDistance = 20000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ | Setting")
	FVector NavigationProjectionExtent = FVector(100.0f, 100.0f, 300.0f);

private:
	bool bRuntimeConfigured = false;
	FSpPlayerCommandResolver Resolver;
	FSpPlayerCommandRuntime_Server ServerRuntime;
	FSpPlayerCommandRuntime_Client ClientRuntime;
	FSpPlayerCommandService_Movement MovementService;

public:
	USpPlayerCommandComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual ~USpPlayerCommandComponent() override = default;
	
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// unit manage
	virtual void OnClearUnit() override;
	virtual void OnUnitActive(bool bActive) override;
	virtual void OnUnitStateChanged(FGameplayTag StateTag, bool bAdded) override;

	// command boundary
	void SubmitCommand_Client(const FSpPlayerCommandInput& CommandInput);
	void HandleCommand_Server(const FSpPlayerCommandInput& CommandInput);
	bool ValidatePredictedAbilityForCommand_Server(uint16 CommandId, FGameplayTag AbilityTag);
	void ApplyMovementState_Client(uint16 CommandId, bool bHasWaypoint, const FVector& Waypoint);

	UFUNCTION(BlueprintPure, Category = "MJ - Command")
	AActor* GetCurrentTargetActor() const { return ServerRuntime.GetTargetActor(); }

private:
	void ConfigureRuntime();
	void ClearCommand();
	void UpdateTickEnabled();
	bool IsGameplayActive_Server() const;
};
