#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SpPlayerState.generated.h"

class USpUnitDefinition;

// ==================================================

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSpSelectedUnitChanged);

// --------------------------------------------------

UCLASS()
class PROJECTSP_API ASpPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, ReplicatedUsing = OnRep_SelectedUnitDefinition, Category = "MJ | Replicate")
	TObjectPtr<USpUnitDefinition> SelectedUnitDefinition;

	UPROPERTY(BlueprintAssignable, Category="MJ - Unit")
	FSpSelectedUnitChanged OnSelectedUnitChanged;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ReadyToStart, Category = "MJ | Replicate")
	bool bReadyToStart = false;

	// ------------------------------------------------
	
	ASpPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	void SetReadyToStart_Server(bool bInReadyToStart);
	void SetSelectedUnitDefinition_Server(USpUnitDefinition* UnitDefinition);

	UFUNCTION(BlueprintPure, Category="MJ - PlayerState")
	USpUnitDefinition* GetUnitDefinition() const;
	
	UFUNCTION(BlueprintPure, Category="MJ - PlayerState")
	bool IsReadyToStart() const { return bReadyToStart; }

private:
	void NotifyLobbyStateChanged();
	
	UFUNCTION()
	void OnRep_ReadyToStart();

	UFUNCTION()
	void OnRep_SelectedUnitDefinition();
};
