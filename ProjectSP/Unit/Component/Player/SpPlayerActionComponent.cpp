#include "SpPlayerActionComponent.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputDefinition.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Input/SpInputBehavior.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/Component/Player/SpPlayerInputComponent.h"

// ==================================================

USpPlayerActionComponent::USpPlayerActionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void USpPlayerActionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (ASpUnit* Unit = GetPawn<ASpUnit>())
	{
		if (USpPlayerInputComponent* InputComponent = Unit->GetUnitComponent<USpPlayerInputComponent>())
		{
			BoundInputComponent = InputComponent;
			InputEventHandle = InputComponent->OnTaggedInput.AddUObject(this, &ThisClass::HandleTaggedInput);
		}
	}
}

void USpPlayerActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (USpPlayerInputComponent* InputComponent = BoundInputComponent.Get())
		InputComponent->OnTaggedInput.Remove(InputEventHandle);

	BoundInputComponent.Reset();
	InputEventHandle.Reset();
	ClearInputState();

	Super::EndPlay(EndPlayReason);
}

void USpPlayerActionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!InputTagToReinterpret.IsValid() && !ActiveBehaviorInputTag.IsValid())
		return;

	ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	if (!Unit || Unit->CheckDead())
		return;

	ASpPlayerController* PC = Unit->GetController<ASpPlayerController>();
	if (!PC || !PC->IsLocalController() || !PC->IsGameplayInputEnabled_Client())
		return;

	ProcessDeferredHeldInput(*Unit);
	TickActiveBehavior(*Unit, DeltaTime);
}

// unit manage

void USpPlayerActionComponent::OnClearUnit()
{
	ClearInputState();
}

void USpPlayerActionComponent::OnUnitActive(const bool bActive)
{
	if (!bActive)
		ClearInputState();
}

void USpPlayerActionComponent::OnUnitPlayable(const bool bPlayable)
{
	if (!bPlayable)
		ClearInputState();
}

// active behavior

void USpPlayerActionComponent::SetInputBehavior(FGameplayTag SourceInputTag, USpInputBehavior& Behavior)
{
	if (!SourceInputTag.IsValid())
		return;

	ClearReinterpretState();

	// 기존 behavior 날리기
	if (ActiveBehavior && ActiveBehavior.Get() != &Behavior)
		ActiveBehavior->ClearBehavior(GetPawn<ASpPlayerUnit>());
	
	ActiveBehaviorInputTag = SourceInputTag;
	ActiveBehavior = &Behavior;
}

void USpPlayerActionComponent::ClearInputBehaviorState()
{
	if (ActiveBehavior)
	{
		ActiveBehavior->ClearBehavior(GetPawn<ASpPlayerUnit>());
		ActiveBehavior = nullptr;
	}

	if (ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>())
		Unit->SetTargetActor(nullptr);

	ActiveBehaviorInputTag = FGameplayTag();
	SyncTargetToServer();
}

// handle input

void USpPlayerActionComponent::HandleTaggedInput(const FGameplayTag InputTag, const FInputActionValue& Value, const ETriggerEvent TriggerEvent)
{
	if (!InputTag.IsValid())
		return;

	ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	if (!Unit || Unit->CheckDead())
		return;

	if (!ApplyHeldTag(InputTag, TriggerEvent)) 
		return;

	if (TriggerEvent == ETriggerEvent::Started || TriggerEvent == ETriggerEvent::Triggered)
		HeldInputValues.Add(InputTag, Value);
	else
		HeldInputValues.Remove(InputTag);

	if (InputTag == InputTagToReinterpret && AbilityTagBlockingReinterpret.IsValid())
	{
		const USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent();
		if (ASC && ASC->IsAbilityTagActive(AbilityTagBlockingReinterpret))
			return;
	}

	const USpInputDefinition* InputDefinition = GetInputDefinition();
	const FSpInputActionBinding* Binding = InputDefinition ? InputDefinition->FindInputBindingForTag(InputTag) : nullptr;

	if (!Binding)
		return;

	FGameplayTag InterruptedInputTag;
	if (TriggerEvent == ETriggerEvent::Started && Binding->bClearStateOnNewInput)
	{
		InterruptedInputTag = ActiveBehaviorInputTag.IsValid() ? ActiveBehaviorInputTag : InputTagToReinterpret;
		if (InterruptedInputTag == InputTag || !IsInputHeld(InterruptedInputTag))
			InterruptedInputTag = FGameplayTag();

		ClearReinterpretState();
		ClearInputBehaviorState();
	}

	bool bBlockReinterpret = false;

	// behavior가 있으면 behavior로 연결
	if (Binding->BehaviorClass)
	{
		if (USpInputBehavior* Behavior = GetInputBehavior(*Binding))
		{
			const bool bIsActiveBehavior = ActiveBehavior.Get() == Behavior && ActiveBehaviorInputTag == InputTag;
			const ESpInputBehaviorRequest StateChange = Behavior->HandleInput(*Unit, Value, TriggerEvent, *Binding, bIsActiveBehavior);
			ApplyBehaviorResult(*Behavior, InputTag, StateChange);
		}

		if (const USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent())
			bBlockReinterpret = ASC->IsAbilityTagActive(Binding->AbilityTag);
	}
	// behavior가 없는 경우 연결된 ability를 try activate
	else if (TriggerEvent == ETriggerEvent::Started && Binding->AbilityTag.IsValid())
	{
		if (USpAbilitySystemComponent* ASC = Unit->GetSpAbilitySystemComponent())
			bBlockReinterpret = ASC->TryActivateAbilityByTag(Binding->AbilityTag);
	}
	else
		return;
	
	// 재해석 대기
	if (InterruptedInputTag.IsValid() && IsInputHeld(InterruptedInputTag) && !ActiveBehaviorInputTag.IsValid())
	{
		InputTagToReinterpret = InterruptedInputTag;
		if (bBlockReinterpret)
			AbilityTagBlockingReinterpret = Binding->AbilityTag;
	}
}

bool USpPlayerActionComponent::ApplyHeldTag(const FGameplayTag InputTag, const ETriggerEvent TriggerEvent)
{
	const bool bWasHeld = HeldInputTags.HasTagExact(InputTag);
	switch (TriggerEvent)
	{
		case ETriggerEvent::Started:
		{
			HeldInputTags.AddTag(InputTag);
			break;
		}
		case ETriggerEvent::Triggered:
		{
			if (!bWasHeld)
				return false;

			break;
		}
		case ETriggerEvent::Completed:
		case ETriggerEvent::Canceled:
		{
			if (!bWasHeld)
				return false;

			HeldInputTags.RemoveTag(InputTag);
			break;
		}

		default:
			return false;
	}

	return true;
}

void USpPlayerActionComponent::ApplyBehaviorResult(USpInputBehavior& Behavior, const FGameplayTag SourceInputTag, const ESpInputBehaviorRequest StateChange)
{
	switch (StateChange)
	{
		case ESpInputBehaviorRequest::Set:
		{
			if (!SourceInputTag.IsValid())
				break;

			SetInputBehavior(SourceInputTag, Behavior);
			SyncTargetToServer();

			break;
		}

		case ESpInputBehaviorRequest::Clear:
		{
			if (ActiveBehavior.Get() == &Behavior)
				ClearInputBehaviorState();

			break;
		}

		default:
			break;
	}
}

// process action

void USpPlayerActionComponent::ProcessDeferredHeldInput(ASpPlayerUnit& Unit)
{
	if (!InputTagToReinterpret.IsValid())
		return;

	const FGameplayTag InputTag = InputTagToReinterpret;
	if (!IsInputHeld(InputTag))
	{
		ClearReinterpretState();
		return;
	}

	if (AbilityTagBlockingReinterpret.IsValid())
	{
		const USpAbilitySystemComponent* ASC = Unit.GetSpAbilitySystemComponent();
		if (ASC && ASC->IsAbilityTagActive(AbilityTagBlockingReinterpret))
			return;
	}

	ClearReinterpretState();

	const USpInputDefinition* InputDefinition = GetInputDefinition();
	const FSpInputActionBinding* Binding = InputDefinition ? InputDefinition->FindInputBindingForTag(InputTag) : nullptr;
	const FInputActionValue* Value = HeldInputValues.Find(InputTag);

	if (!Binding || !Binding->BehaviorClass || !Value)
		return;

	if (USpInputBehavior* Behavior = GetInputBehavior(*Binding))
		ApplyBehaviorResult(*Behavior, InputTag, Behavior->HandleInput(Unit, *Value, ETriggerEvent::Started, *Binding, false));
}

void USpPlayerActionComponent::TickActiveBehavior(ASpPlayerUnit& Unit, const float DeltaTime)
{
	if (!ActiveBehaviorInputTag.IsValid() || !ActiveBehavior)
		return;

	ActiveBehavior->TickBehavior(Unit, *this, ActiveBehaviorInputTag, DeltaTime);
}

// clear

void USpPlayerActionComponent::ClearReinterpretState()
{
	InputTagToReinterpret = FGameplayTag();
	AbilityTagBlockingReinterpret = FGameplayTag();
}

void USpPlayerActionComponent::ClearInputState()
{
	ClearInputBehaviorState();
	ClearReinterpretState();

	HeldInputTags = FGameplayTagContainer();
	HeldInputValues.Empty();
	InputBehaviors.Empty();
}

// target

void USpPlayerActionComponent::SyncTargetToServer(const bool bFaceTarget)
{
	ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();

	if (Unit && !Unit->HasAuthority() && Unit->IsLocallyControlled())
		ServerSetTargetActor(Unit->GetTargetActor(), bFaceTarget);
}

void USpPlayerActionComponent::ServerSetTargetActor_Implementation(ASpUnit* InTargetActor, const bool bFaceTarget)
{
	ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	if (!Unit)
		return;

	const bool bAttackable = Unit->IsAttackable(InTargetActor);
	Unit->SetTargetActor(bAttackable ? InTargetActor : nullptr);
	
	if (bFaceTarget && bAttackable)
	{
		FVector ToTarget = InTargetActor->GetActorLocation() - Unit->GetActorLocation();
		ToTarget.Z = 0.0f;
		
		if (!ToTarget.IsNearlyZero())
		{
			FRotator FacingRotation = Unit->GetActorRotation();
			FacingRotation.Yaw = ToTarget.Rotation().Yaw;
			Unit->SetActorRotation(FacingRotation);
		}
	}
}

// get

bool USpPlayerActionComponent::IsInputHeld(FGameplayTag InputTag) const
{
	return HeldInputTags.HasTagExact(InputTag);
}

ASpUnit* USpPlayerActionComponent::GetTargetActor() const
{
	const ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	return Unit ? Unit->GetTargetActor() : nullptr;
}

const USpInputDefinition* USpPlayerActionComponent::GetInputDefinition() const
{
	const ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	const USpUnitDefinition* UnitDefinition = Unit ? Unit->GetUnitDefinition() : nullptr;
	return UnitDefinition ? UnitDefinition->InputDefinition : nullptr;
}

USpInputBehavior* USpPlayerActionComponent::GetInputBehavior(const FSpInputActionBinding& Binding)
{
	if (!Binding.BehaviorClass || Binding.BehaviorClass->HasAnyClassFlags(CLASS_Abstract))
		return nullptr;

	if (TObjectPtr<USpInputBehavior>* Behavior = InputBehaviors.Find(Binding.InputTag))
	{
		if (*Behavior && (*Behavior)->IsA(Binding.BehaviorClass))
			return Behavior->Get();
	}

	USpInputBehavior* Behavior = NewObject<USpInputBehavior>(this, Binding.BehaviorClass);
	InputBehaviors.Add(Binding.InputTag, Behavior);

	return Behavior;
}
