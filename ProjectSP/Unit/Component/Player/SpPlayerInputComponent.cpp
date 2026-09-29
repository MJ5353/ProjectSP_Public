#include "SpPlayerInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputDefinition.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Input/SpInputComponent.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

void USpPlayerInputComponent::SetUp(UInputComponent* InputComponent)
{
	if (bIsInputInitialized || !InputComponent)
		return;

	ASpPlayerController* PC = GetController<ASpPlayerController>();
	if (!PC || !PC->IsLocalController())
		return;

	PC->bShowMouseCursor = true;
	PC->DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	const ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	const USpUnitDefinition* UnitDefinition = Unit ? Unit->GetUnitDefinition() : nullptr;
	if (!UnitDefinition || !UnitDefinition->InputDefinition)
		return;

	USpInputComponent* SpInputComponent = Cast<USpInputComponent>(InputComponent);
	if (!SpInputComponent)
		return;

	if (!PC->ApplyUnitInputMappings_Client(UnitDefinition))
		return;

	for (const FSpInputActionBinding& Binding : UnitDefinition->InputDefinition->InputActions)
	{
		if (!Binding.InputAction || !Binding.InputTag.IsValid())
			continue;

		SpInputComponent->BindAction(Binding.InputAction, ETriggerEvent::Started, this, &ThisClass::InputStarted, Binding.InputTag);

		if (Binding.bSendTriggeredUpdates)
			SpInputComponent->BindAction(Binding.InputAction, ETriggerEvent::Triggered, this, &ThisClass::InputTriggered, Binding.InputTag);
		
		SpInputComponent->BindAction(Binding.InputAction, ETriggerEvent::Completed, this, &ThisClass::InputCompleted, Binding.InputTag);
		SpInputComponent->BindAction(Binding.InputAction, ETriggerEvent::Canceled, this, &ThisClass::InputCanceled, Binding.InputTag);
	}

	bIsInputInitialized = true;
}

// input action

void USpPlayerInputComponent::InputStarted(const FInputActionValue& Value, const FGameplayTag InputTag)
{
	if (IsGameplayInputEnabled_Client())
		OnTaggedInput.Broadcast(InputTag, Value, ETriggerEvent::Started);
}

void USpPlayerInputComponent::InputTriggered(const FInputActionValue& Value, const FGameplayTag InputTag)
{
	if (IsGameplayInputEnabled_Client())
		OnTaggedInput.Broadcast(InputTag, Value, ETriggerEvent::Triggered);
}

void USpPlayerInputComponent::InputCompleted(const FInputActionValue& Value, const FGameplayTag InputTag)
{
	OnTaggedInput.Broadcast(InputTag, Value, ETriggerEvent::Completed);
}

void USpPlayerInputComponent::InputCanceled(const FInputActionValue& Value, const FGameplayTag InputTag)
{
	OnTaggedInput.Broadcast(InputTag, Value, ETriggerEvent::Canceled);
}

// get

bool USpPlayerInputComponent::IsGameplayInputEnabled_Client() const
{
	if (ASpPlayerController* PC = GetController<ASpPlayerController>())
		return PC->IsLocalController() && PC->IsGameplayInputEnabled_Client();
	
	return false;
}
