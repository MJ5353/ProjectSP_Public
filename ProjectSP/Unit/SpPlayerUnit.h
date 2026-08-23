#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "SpPlayerUnit.generated.h"

class USpUnitCameraComponent;
class USpUnitInputComponent;
class USpPlayerCommandComponent;
class USpUnitStimuliSourceComponent;
class USpAbilitySystemComponent;

// ==================================================

UCLASS()
class PROJECTSP_API ASpPlayerUnit : public ASpUnit
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpUnitInputComponent> UnitInputComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MJ - Component", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpPlayerCommandComponent> PlayerCommandComponent;

public:
	ASpPlayerUnit(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	// get
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual USpAbilitySystemComponent* GetSpAbilitySystemComponent() const override;
	
	USpPlayerCommandComponent* GetPlayerCommandComponent() const { return PlayerCommandComponent; }

private:
	void InitializeAbilitySystem();
	void TrySetInput();
};
