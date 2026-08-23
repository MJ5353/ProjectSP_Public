#include "SpUnitInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputMappingContext.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/GameFramework/SpPlayerState.h"
#include "ProjectSP/Input/SpInputComponent.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Component/Player/SpPlayerCommandComponent.h"

// ==================================================

USpUnitInputComponent::USpUnitInputComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USpUnitInputComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsGameplayInputEnabled_Client())
		return;
	
#if !UE_BUILD_SHIPPING
	if (APlayerController* PC = GetController<APlayerController>())
	{
		if (PC->WasInputKeyJustPressed(EKeys::L))
		{
			FGameplayTag Tag = FSpGameplayTags::Get().AbilitySlotTag_BasicAttack;
			float OutRemain =0.f;
			float OutDuration =0.f;
			
			GetPawn<ASpUnit>()->GetSpAbilitySystemComponent()->GetAbilityCooldownByTag(Tag,OutRemain, OutDuration);
			UE_LOG(LogGameplayTags, Log, TEXT("[mj] Remain Cool : %f"), OutDuration);
		}
	}
#endif
}

// unit manage

void USpUnitInputComponent::OnClearUnit()
{
	EndPrimaryCommand_Client();
	ClearCommand();
	SetComponentTickEnabled(false);
}

void USpUnitInputComponent::OnUnitActive(bool bActive)
{
	if (!bActive)
		EndPrimaryCommand_Client();

	SetComponentTickEnabled(bActive);
}

// set

void USpUnitInputComponent::SetUp(UInputComponent* InputComponent)
{
	if (bIsInputInitialized || !InputComponent)
		return;

	APlayerController* PC = GetController<APlayerController>();
	if (!PC || !PC->IsLocalController())
		return;

	// 커서 보이기
	PC->bShowMouseCursor = true;
	PC->DefaultMouseCursor = EMouseCursor::Default;

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PC->SetInputMode(InputMode);

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
		return;

	UEnhancedInputLocalPlayerSubsystem* InputSubSystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	if (!InputSubSystem)
		return;

	ASpPlayerState* PS = PC->GetPlayerState<ASpPlayerState>();
	if (!PS)
		return;

	USpUnitDefinition* UnitDefinition = PS->UnitDefinition;
	if (!UnitDefinition)
		return;

	USpInputDefinition* InputDefinition = UnitDefinition->InputDefinition;
	if (!InputDefinition)
		return;

	USpInputComponent* InputComp = Cast<USpInputComponent>(InputComponent);
	if (!InputComp)
		return;

	InputSubSystem->ClearAllMappings();

	// mapping context
	const TArray<FSpInputMappingContext>& InputMappingContexts = UnitDefinition->InputMappingContexts;
	for (const FSpInputMappingContext& MappingContext : InputMappingContexts)
	{
		if (MappingContext.bShouldActivateAutomatically == false)
			continue;

		FModifyContextOptions Options = {};
		Options.bIgnoreAllPressedKeysUntilRelease = false;

		const UInputMappingContext* Context = MappingContext.Context;
		InputSubSystem->AddMappingContext(Context, 0, Options);
	}

	for (uint32 Handle : AbilityInputBindHandles)
	{
		InputComp->RemoveBindingByHandle(Handle);
	}
	AbilityInputBindHandles.Reset();

	// ability tag 
	InputComp->BindAbilityActions(InputDefinition, this, &ThisClass::InputAbilityTag_Pressed, &ThisClass::InputAbilityTag_Released, AbilityInputBindHandles);

	// native tag
	const FSpGameplayTags& Tags = FSpGameplayTags::Get();

	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_PrimaryClick, ETriggerEvent::Started, this, &ThisClass::InputPrimaryClickStarted);
	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_PrimaryClick, ETriggerEvent::Triggered, this, &ThisClass::InputPrimaryClickTriggered);
	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_PrimaryClick, ETriggerEvent::Completed, this, &ThisClass::InputPrimaryClickEnded);
	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_PrimaryClick, ETriggerEvent::Canceled, this, &ThisClass::InputPrimaryClickEnded);

	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_SecondaryClick, ETriggerEvent::Triggered, this, &ThisClass::InputSecondaryClick);
	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_Move_WASD, ETriggerEvent::Triggered, this, &ThisClass::InputMove_WASD);
	InputComp->BindNativeAction(InputDefinition, Tags.InputTag_Look_Mouse, ETriggerEvent::Triggered, this, &ThisClass::InputLook_Mouse);

	bIsInputInitialized = true;
}

// check

bool USpUnitInputComponent::IsGameplayInputEnabled_Client() const
{
	const ASpPlayerController* PlayerController = GetController<ASpPlayerController>();
	return PlayerController && PlayerController->IsLocalController() && PlayerController->IsGameplayInputEnabled_Client();
}

// ------------------------------------------------

// client

void USpUnitInputComponent::EndPrimaryCommand_Client()
{
	if (ActivePrimaryCommandId == 0)
		return;

	if (ASpUnit* Unit = GetPawn<ASpUnit>())
	{
		FSpPlayerCommandInput CommandInput;
		CommandInput.CommandId = ActivePrimaryCommandId;
		CommandInput.Phase = ESpPlayerCommandInputPhase::End;
		
		SubmitCommand_Client(Unit, CommandInput);
	}

	ClearCommand();
}

// command

uint16 USpUnitInputComponent::AllocatePrimaryCommandId()
{
	++NextPrimaryCommandId;
	
	if (NextPrimaryCommandId == 0)
		++NextPrimaryCommandId;
	
	return NextPrimaryCommandId;
}

void USpUnitInputComponent::SubmitCommand_Client(const ASpUnit* Unit, const FSpPlayerCommandInput& CommandInput)
{
	if (USpPlayerCommandComponent* CommandComponent = Unit->GetUnitComponent<USpPlayerCommandComponent>())
	{
		CommandComponent->SubmitCommand_Client(CommandInput);
	}
	else if (USpAbilitySystemComponent* AbilitySystemComponent = Unit->GetSpAbilitySystemComponent())
	{
		AbilitySystemComponent->SubmitPlayerCommand_Client(CommandInput);
	}
}

void USpUnitInputComponent::ClearCommand()
{
	ActivePrimaryCommandId = 0;
	CursorUpdateSequence = 0;
	LastCursorUpdateTime = -1.0f;
}

// helper

bool USpUnitInputComponent::BuildCursorRay(FSpPlayerCommandCursorRay& OutCursorRay) const
{
	ASpPlayerController* PlayerController = GetController<ASpPlayerController>();
	if (!PlayerController)
		return false;

	FVector RayOrigin;
	FVector RayDirection;
	if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
		return false;

	OutCursorRay.Origin = RayOrigin;
	OutCursorRay.Direction = RayDirection.GetSafeNormal();
	return !OutCursorRay.Direction.IsNearlyZero();
}

// input callback

void USpUnitInputComponent::InputPrimaryClickStarted(const FInputActionValue& InputActionValue)
{
	if (!IsGameplayInputEnabled_Client())
		return;

	ASpPlayerController* PlayerController = GetController<ASpPlayerController>();
	FSpPlayerCommandCursorRay CursorRay;
	
	if (!PlayerController || !BuildCursorRay(CursorRay))
		return;
	
	EndPrimaryCommand_Client();
	
	ActivePrimaryCommandId = AllocatePrimaryCommandId();
	CursorUpdateSequence = 0;
	LastCursorUpdateTime = GetWorld() ? GetWorld()->GetTimeSeconds() : -1.0f;
	
	if (ASpUnit* Unit = GetPawn<ASpUnit>())
	{
		FSpPlayerCommandInput CommandInput;
		CommandInput.CommandId = ActivePrimaryCommandId;
		CommandInput.Phase = ESpPlayerCommandInputPhase::Begin;
		CommandInput.CursorRay = CursorRay;

		SubmitCommand_Client(Unit, CommandInput);
	}
}

void USpUnitInputComponent::InputPrimaryClickTriggered(const FInputActionValue& InputActionValue)
{
	if (!IsGameplayInputEnabled_Client())
		return;

	if (ActivePrimaryCommandId == 0)
		return;

	UWorld* World = GetWorld();
	ASpPlayerController* PlayerController = GetController<ASpPlayerController>();
	FSpPlayerCommandCursorRay CursorRay;
	if (!World || !PlayerController || !BuildCursorRay(CursorRay))
		return;

	constexpr float CursorUpdateInterval = 1.0f / 15.0f;
	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime - LastCursorUpdateTime < CursorUpdateInterval)
		return;

	LastCursorUpdateTime = CurrentTime;
	++CursorUpdateSequence;

	if (ASpUnit* Unit = GetPawn<ASpUnit>())
	{
		FSpPlayerCommandInput CommandInput;
		CommandInput.CommandId = ActivePrimaryCommandId;
		CommandInput.Phase = ESpPlayerCommandInputPhase::Update;
		CommandInput.Sequence = CursorUpdateSequence;
		CommandInput.CursorRay = CursorRay;

		SubmitCommand_Client(Unit, CommandInput);
	}
}

void USpUnitInputComponent::InputPrimaryClickEnded(const FInputActionValue& InputActionValue)
{
	EndPrimaryCommand_Client();
}

void USpUnitInputComponent::InputSecondaryClick(const FInputActionValue& InputActionValue)
{
	if (!IsGameplayInputEnabled_Client())
		return;
	
	// [mj] todo) 
}

// input callback - ability

void USpUnitInputComponent::InputAbilityTag_Pressed(FGameplayTag InputTag)
{
	if (!IsGameplayInputEnabled_Client())
		return;

	if (!InputTag.IsValid())
		return;
	
	ASpUnit* Unit = GetPawnChecked<ASpUnit>();

	if (USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent())
		ASC->OnAbilityInputTagPressed(InputTag);
}

void USpUnitInputComponent::InputAbilityTag_Released(FGameplayTag InputTag)
{
	if (!InputTag.IsValid())
		return;
	
	ASpUnit* Unit = GetPawnChecked<ASpUnit>();

	if (USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent())
		ASC->OnAbilityInputTagReleased(InputTag);
}

void USpUnitInputComponent::InputMove_WASD(const FInputActionValue& InputActionValue)
{
	if (!IsGameplayInputEnabled_Client())
		return;

	APawn* Pawn = GetPawnChecked<APawn>();

	AController* Controller = Pawn->GetController();
	if (ensure(Controller) == false)
		return;

	const FVector2D Value = InputActionValue.Get<FVector2D>();
	const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);

	if (Value.X != 0.0f)
	{
		// Left/Right -> X 값에 들어있음:
		// MovementDirection은 현재 카메라의 RightVector를 의미함 (World-Space)
		const FVector MovementDirection = MovementRotation.RotateVector(FVector::RightVector);

		// AddMovementInput 함수를 한번 보자:
		// - 내부적으로 MovementDirection * Value.X를 MovementComponent에 적용(더하기)해준다
		Pawn->AddMovementInput(MovementDirection, Value.X);
	}

	// Forward 적용을 위해 swizzle input modifier를 사용함
	if (Value.Y != 0.0f)
	{
		// Left/Right와 마찬가지로 Forward/Backward를 적용한다
		const FVector MovementDirection = MovementRotation.RotateVector(FVector::ForwardVector);
		Pawn->AddMovementInput(MovementDirection, Value.Y);
	}
}

void USpUnitInputComponent::InputLook_Mouse(const FInputActionValue& InputActionValue)
{
	if (!IsGameplayInputEnabled_Client())
		return;

	APawn* Pawn = GetPawn<APawn>();
	if (!Pawn)
		return;

	const FVector2D Value = InputActionValue.Get<FVector2D>();
	if (Value.X != 0.0f)
	{
		// X에는 Yaw 값이 있음:
		// - Camera에 대해 Yaw 적용
		Pawn->AddControllerYawInput(Value.X);
	}

	if (Value.Y != 0.0f)
	{
		// Y에는 Pitch 값!
		double AimInversionValue = -Value.Y;
		Pawn->AddControllerPitchInput(AimInversionValue);
	}
}


