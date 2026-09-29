#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "SpPlayerUnit.generated.h"

class USpPlayerCameraComponent;
class USpPlayerInputComponent;
class USpPlayerActionComponent;
class USpUnitStimuliSourceComponent;

// ==================================================

UCLASS()
class PROJECTSP_API ASpPlayerUnit : public ASpUnit
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpPlayerCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpPlayerInputComponent> UnitInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpPlayerActionComponent> PlayerActionComponent;

	FVector SpawnLocation = FVector::ZeroVector;

public:
	ASpPlayerUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	virtual void OnRep_UnitData() override;
	virtual void HandleDeadProcessFinished_Server() override;
	void TrySetInput(bool bInitUnit);

public:
	UFUNCTION(BlueprintPure)
	USpPlayerActionComponent* GetPlayerActionComponent() const { return PlayerActionComponent; }

	FVector GetSpawnLocation() const { return SpawnLocation; }
};
