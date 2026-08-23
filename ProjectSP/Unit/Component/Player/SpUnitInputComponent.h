#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/PawnComponent.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandTypes.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpUnitInputComponent.generated.h"

class ASpUnit;
struct FGameplayTag;
struct FInputActionValue;
struct FSpInputMappingContext;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitInputComponent : public UPawnComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

protected:
	TArray<uint32> AbilityInputBindHandles; // 재할당 방지
	bool bIsInputInitialized = false;
	
	uint16 NextPrimaryCommandId = 0;
	uint16 ActivePrimaryCommandId = 0;
	uint16 CursorUpdateSequence = 0;
	float LastCursorUpdateTime = -1.0f;
	
public:
	USpUnitInputComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// unit manage
	virtual void OnClearUnit() override;
	virtual void OnUnitActive(bool bActive) override;
	
	// set
	void SetUp(UInputComponent* InputComponent);
	
	// check
	bool IsGameplayInputEnabled_Client() const;

protected:
	// client
	void EndPrimaryCommand_Client();
	
	// command
	uint16 AllocatePrimaryCommandId();
	void SubmitCommand_Client(const ASpUnit* Unit, const FSpPlayerCommandInput& CommandInput);
	void ClearCommand();

	// helper
	bool BuildCursorRay(FSpPlayerCommandCursorRay& OutCursorRay) const;

	// input callback
	void InputPrimaryClickStarted(const FInputActionValue& InputActionValue);
	void InputPrimaryClickTriggered(const FInputActionValue& InputActionValue);
	void InputPrimaryClickEnded(const FInputActionValue& InputActionValue);
	void InputSecondaryClick(const FInputActionValue& InputActionValue);
	
	// input callback - ability
	void InputAbilityTag_Pressed(FGameplayTag InputTag);
	void InputAbilityTag_Released(FGameplayTag InputTag);
	void InputMove_WASD(const FInputActionValue& InputActionValue);
	void InputLook_Mouse(const FInputActionValue& InputActionValue);
};

