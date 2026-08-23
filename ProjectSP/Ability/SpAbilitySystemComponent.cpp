#include "SpAbilitySystemComponent.h"
#include "SpGameplayAbility.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "ProjectSP/Animation/SpAnimInstance.h"
#include "ProjectSP/Attribute/SpHPAttributeSet.h"
#include "ProjectSP/Definition/GameplayAbility/SpAbilityDefinition.h"
#include "ProjectSP/Definition/GameplayAbility/SpAttributeDefinition.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/Component/Player/SpPlayerCommandComponent.h"
#include "TimerManager.h"

// ==================================================

namespace
{
	constexpr float PlayerStateMontageReplicationNetUpdateFrequency = 100.0f;
	constexpr float PlayerStateMontageReplicationRestoreDelay = 0.1f;
}

// init

void USpAbilitySystemComponent::SetUpAbilityAndAttribute(const USpUnitDefinition& UnitDefinition)
{
	for (const USpAbilityDefinition* AbilityDefinition : UnitDefinition.AbilityDefinitions)
	{
		if (AbilityDefinition)
			AbilityDefinition->GiveToAbilitySystem(this, &GrantedAbilityHandles);
	}

	if (UnitDefinition.AttributeDefinition)
	{
		UnitDefinition.AttributeDefinition->GiveToAbilitySystem(this, &GrantedAttributeSets);
		RestoreHp();
	}
}

void USpAbilitySystemComponent::TryActivateOnSpawnAbilities()
{
	if (bOnSpawnAbilitiesActivated || !IsOwnerActorAuthoritative())
		return;

	bOnSpawnAbilitiesActivated = true;

	for (FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		const USpGameplayAbility* AbilityCDO = Cast<USpGameplayAbility>(AbilitySpec.Ability);
		if (!AbilityCDO || AbilityCDO->ActivationPolicy != EMjAbilityActivationPolicy::OnSpawn)
			continue;

		TryActivateAbility(AbilitySpec.Handle);
	}
}

void USpAbilitySystemComponent::BeginPlayerStateMontageReplicationBoost()
{
	if (!IsOwnerActorAuthoritative())
		return;

	APlayerState* PlayerState = Cast<APlayerState>(GetOwnerActor());
	if (!PlayerState)
		return;

	if (bPlayerStateMontageReplicationBoostActive && MontageReplicationPlayerState.Get() != PlayerState)
		RestorePlayerStateMontageReplicationRate();

	ClearPlayerStateMontageReplicationRestoreTimer();

	if (!bPlayerStateMontageReplicationBoostActive)
	{
		bPlayerStateMontageReplicationBoostActive = true;
		MontageReplicationPlayerState = PlayerState;
		SavedPlayerStateNetUpdateFrequency = PlayerState->GetNetUpdateFrequency();
		PlayerState->SetNetUpdateFrequency(FMath::Max(SavedPlayerStateNetUpdateFrequency, PlayerStateMontageReplicationNetUpdateFrequency));
	}

	// 기본 GAS 구현은 Avatar를 즉시 갱신한다. PlayerState 소유 ASC는 Owner도 갱신해야 한다.
	PlayerState->ForceNetUpdate();
}

void USpAbilitySystemComponent::SchedulePlayerStateMontageReplicationRestore()
{
	if (!bPlayerStateMontageReplicationBoostActive)
		return;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			MontageReplicationRestoreTimer,
			this,
			&ThisClass::RestorePlayerStateMontageReplicationRate,
			PlayerStateMontageReplicationRestoreDelay,
			false);
	}
	else
	{
		RestorePlayerStateMontageReplicationRate();
	}
}

void USpAbilitySystemComponent::RestorePlayerStateMontageReplicationRate()
{
	if (!bPlayerStateMontageReplicationBoostActive || GetAnimatingAbility())
		return;

	ClearPlayerStateMontageReplicationRestoreTimer();

	if (AActor* PlayerState = MontageReplicationPlayerState.Get())
	{
		PlayerState->SetNetUpdateFrequency(SavedPlayerStateNetUpdateFrequency);
		PlayerState->ForceNetUpdate();
	}

	bPlayerStateMontageReplicationBoostActive = false;
	SavedPlayerStateNetUpdateFrequency = 0.0f;
	MontageReplicationPlayerState.Reset();
}

void USpAbilitySystemComponent::ClearPlayerStateMontageReplicationRestoreTimer()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(MontageReplicationRestoreTimer);
}

void USpAbilitySystemComponent::RestoreHp()
{
	if (!IsOwnerActorAuthoritative())
		return;

	if (!GetSet<USpHPAttributeSet>())
		return;

	const float MaxHP = GetNumericAttribute(USpHPAttributeSet::GetMaxHPAttribute());
	SetNumericAttributeBase(USpHPAttributeSet::GetCurrentHPAttribute(), MaxHP);
}

void USpAbilitySystemComponent::NotifyDamageApplied(AActor* Instigator, float AppliedDamage)
{
	if (!IsOwnerActorAuthoritative() || AppliedDamage <= 0.0f)
		return;

	OnDamageApplied.Broadcast(Instigator, AppliedDamage);
}

// clear

void USpAbilitySystemComponent::Clear()
{
	if (IsOwnerActorAuthoritative())
	{
		GrantedAbilityHandles.TakeFromAbilitySystem(this);
		GrantedAttributeSets.TakeFromAbilitySystem(this);
	}

	ClearAbilityInput();
}

void USpAbilitySystemComponent::ClearAbilityInput()
{
	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
	InputHeldSpecHandles.Reset();
	AbilitiesToActivate.Reset();

	for (FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		AbilitySpec.InputPressed = false;
	}
}

const FGameplayAbilitySpec* USpAbilitySystemComponent::FindAbilitySpecByTag(const FGameplayTag& Tag) const
{
	if (!Tag.IsValid())
		return nullptr;

	for (const FGameplayAbilitySpec& AbilitySpec : ActivatableAbilities.Items)
	{
		if (AbilitySpec.Handle.IsValid() && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(Tag))
			return &AbilitySpec;
	}

	return nullptr;
}

// virtual

void USpAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	// ability actor info가 변경될 때마다, InitializeWithAbilitySystem를 호출하여 혹시 모를 순서 이슈에 대비
	// InitializeWithAbilitySystem은 Tag - AnimInstance mapping 갱신 함수

	FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	check(ActorInfo);
	check(InOwnerActor);

	const bool bHasNewUnitAvatar = Cast<APawn>(InAvatarActor) && InAvatarActor != ActorInfo->AvatarActor;
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	if (ASpUnit* Unit = Cast<ASpUnit>(InAvatarActor))
		Unit->ApplyRotationRateFromAttribute();

	if (bHasNewUnitAvatar)
	{
		if (USpAnimInstance* AnimInstance = Cast<USpAnimInstance>(ActorInfo->GetAnimInstance()))
			AnimInstance->InitializeWithAbilitySystem(this);
	}
}

void USpAbilitySystemComponent::AbilitySpecInputPressed(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputPressed(Spec);

	if (!Spec.IsActive())
		return;

	UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
	if (!AbilityInstance)
		return;

	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
}

void USpAbilitySystemComponent::AbilitySpecInputReleased(FGameplayAbilitySpec& Spec)
{
	Super::AbilitySpecInputReleased(Spec);

	if (!Spec.IsActive())
		return;

	UGameplayAbility* AbilityInstance = Spec.GetPrimaryInstance();
	if (!AbilityInstance)
		return;

	InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, Spec.Handle, AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey());
}

float USpAbilitySystemComponent::PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, const float InPlayRate, const FName StartSectionName, const float StartTimeSeconds)
{
	const float Duration = Super::PlayMontage(AnimatingAbility, ActivationInfo, Montage, InPlayRate, StartSectionName, StartTimeSeconds);

	if (Duration > 0.0f)
		BeginPlayerStateMontageReplicationBoost();

	return Duration;
}

void USpAbilitySystemComponent::ClearAnimatingAbility(UGameplayAbility* Ability)
{
	Super::ClearAnimatingAbility(Ability);

	if (IsOwnerActorAuthoritative() && !GetAnimatingAbility())
		SchedulePlayerStateMontageReplicationRestore();
}

void USpAbilitySystemComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPlayerStateMontageReplicationRestoreTimer();

	if (bPlayerStateMontageReplicationBoostActive)
	{
		if (AActor* PlayerState = MontageReplicationPlayerState.Get())
			PlayerState->SetNetUpdateFrequency(SavedPlayerStateNetUpdateFrequency);

		bPlayerStateMontageReplicationBoostActive = false;
		SavedPlayerStateNetUpdateFrequency = 0.0f;
		MontageReplicationPlayerState.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

// state

void USpAbilitySystemComponent::OnUnitActive(bool bActive)
{
	if (!bActive)
	{
		ResetForPool();
		return;
	}
	
	ASpUnit* Unit = GetOwner<ASpUnit>();
	if (!Unit)
		return; 
	
	const USpUnitDefinition* UnitDefinition = Unit->GetUnitDefinition();
	if (!UnitDefinition)
		return;
	
	SetUp(UnitDefinition);
	InitAbilityActorInfo(Unit, Unit);
}

// set

void USpAbilitySystemComponent::SetUp(const USpUnitDefinition* UnitDefinition)
{
	if (!IsOwnerActorAuthoritative() || !UnitDefinition)
		return;

	Clear();
	SetUpAbilityAndAttribute(*UnitDefinition);
	//TryActivateOnSpawnAbilities();
}

void USpAbilitySystemComponent::ResetForPool()
{
	CancelAllAbilities();
	ClearAbilityInput();

	if (IsOwnerActorAuthoritative())
		RemoveActiveEffects(FGameplayEffectQuery());

	bOnSpawnAbilitiesActivated = false;
}

// attribute

void USpAbilitySystemComponent::UnregisterAttributeChangeCallback(const FGameplayAttribute& Attribute, FDelegateHandle& Handle)
{
	if (!Attribute.IsValid() || !Handle.IsValid())
		return;

	FOnGameplayAttributeValueChange& ChangeDelegate = GetGameplayAttributeValueChangeDelegate(Attribute);
	ChangeDelegate.Remove(Handle);
	Handle.Reset();
}

// ability input

void USpAbilitySystemComponent::OnAbilityInputTagPressed(FGameplayTag InputTag)
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(InputTag);
	if (!AbilitySpec)
		return;

	InputPressedSpecHandles.AddUnique(AbilitySpec->Handle);
	InputHeldSpecHandles.AddUnique(AbilitySpec->Handle);
}

void USpAbilitySystemComponent::OnAbilityInputTagReleased(FGameplayTag InputTag)
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(InputTag);
	if (!AbilitySpec)
		return;

	InputReleasedSpecHandles.AddUnique(AbilitySpec->Handle);
	InputHeldSpecHandles.Remove(AbilitySpec->Handle);
}

void USpAbilitySystemComponent::OnProcessAbilityInput(float DeltaTime, bool bGamePaused)
{
	AbilitiesToActivate.Reset();

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : InputHeldSpecHandles)
	{
		if (const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle))
		{
			if (!AbilitySpec->Ability || AbilitySpec->IsActive())
				continue;

			const USpGameplayAbility* AbilityCDO = CastChecked<USpGameplayAbility>(AbilitySpec->Ability);

			if (AbilityCDO->ActivationPolicy == EMjAbilityActivationPolicy::WhileInputActive)
				AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : InputPressedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle))
		{
			if (!AbilitySpec->Ability)
				continue;

			AbilitySpec->InputPressed = true;

			if (AbilitySpec->IsActive())
			{
				AbilitySpecInputPressed(*AbilitySpec);
			}
			else
			{
				const USpGameplayAbility* AbilityCDO = CastChecked<USpGameplayAbility>(AbilitySpec->Ability);

				if (AbilityCDO->ActivationPolicy == EMjAbilityActivationPolicy::OnInputTriggered)
					AbilitiesToActivate.AddUnique(AbilitySpec->Handle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : AbilitiesToActivate)
	{
		// 모든 것이 잘 진행되었다면, CallActivate 호출로 BP의 Activate 노드가 실행될 것
		TryActivateAbility(AbilitySpecHandle);
	}

	// 이번 프레임에 Release된 Ability 처리
	for (const FGameplayAbilitySpecHandle& AbilitySpecHandle : InputReleasedSpecHandles)
	{
		if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilitySpecHandle))
		{
			if (!AbilitySpec->Ability)
				continue;

			AbilitySpec->InputPressed = false;
			if (AbilitySpec->IsActive())
				AbilitySpecInputReleased(*AbilitySpec);
		}
	}

	InputPressedSpecHandles.Reset();
	InputReleasedSpecHandles.Reset();
}

bool USpAbilitySystemComponent::IsAbilityTagActive(FGameplayTag Tag) const
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(Tag);
	if (!AbilitySpec || !AbilitySpec->Ability)
		return false;

	return AbilitySpec->IsActive();
}

// ability tag

bool USpAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag Tag)
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(Tag);
	if (!AbilitySpec || !AbilitySpec->Ability)
		return false;
	
	return TryActivateAbility(AbilitySpec->Handle);
}

bool USpAbilitySystemComponent::CanActivateAbilityByTag(FGameplayTag Tag) const
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(Tag);
	const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	
	if (!AbilitySpec || !AbilitySpec->Ability || !ActorInfo)
		return false;

	return AbilitySpec->Ability->CanActivateAbility(AbilitySpec->Handle, ActorInfo);
}

bool USpAbilitySystemComponent::HasAbilityAssetTag(FGameplayTag AbilityTag, FGameplayTag AssetTag) const
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(AbilityTag);
	if (!AbilitySpec || !AbilitySpec->Ability || !AssetTag.IsValid())
		return false;

	return AbilitySpec->Ability->GetAssetTags().HasTagExact(AssetTag);
}

bool USpAbilitySystemComponent::GetAbilityCooldownByTag(FGameplayTag Tag, float& OutRemaining, float& OutDuration) const
{
	OutRemaining = 0.0f;
	OutDuration = 0.0f;

	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(Tag);
	const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();
	if (!AbilitySpec || !AbilitySpec->Ability || !ActorInfo)
		return false;

	AbilitySpec->Ability->GetCooldownTimeRemainingAndDuration(AbilitySpec->Handle, ActorInfo, OutRemaining, OutDuration);
	return true;
}

// player command

void USpAbilitySystemComponent::SubmitPlayerCommand_Client(const FSpPlayerCommandInput& CommandInput)
{
	if (CommandInput.CommandId == 0)
		return;

	if (CommandInput.Phase == ESpPlayerCommandInputPhase::Update)
		ServerUpdatePlayerCommand(CommandInput);
	else
		ServerSubmitPlayerCommand(CommandInput);
}

bool USpAbilitySystemComponent::TryActivateAbilityForPlayerCommand_Client(const FGameplayTag AbilityTag, const uint16 CommandId)
{
	if (!AbilityTag.IsValid() || CommandId == 0)
		return false;

	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(AbilityTag);
	if (!AbilitySpec || !AbilitySpec->Ability)
		return false;

	const FGameplayAbilitySpecHandle AbilityHandle = AbilitySpec->Handle;
	if (!TryActivateAbility(AbilityHandle))
		return false;

	FGameplayAbilitySpec* ActivatedSpec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!ActivatedSpec || !ActivatedSpec->IsActive())
		return false;
	
	UGameplayAbility* AbilityInstance = ActivatedSpec->GetPrimaryInstance();
	if (!AbilityInstance)
		return false;

	const FPredictionKey ActivationPredictionKey = AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey();
	ServerConfirmPredictedAbilityForCommand(AbilityHandle, ActivationPredictionKey, AbilityTag, CommandId);

	return true;
}

void USpAbilitySystemComponent::ServerSubmitPlayerCommand_Implementation(const FSpPlayerCommandInput CommandInput)
{
	if (CommandInput.CommandId == 0 || CommandInput.Phase == ESpPlayerCommandInputPhase::Update)
		return;

	ASpPlayerUnit* PlayerUnit = Cast<ASpPlayerUnit>(GetAvatarActor());
	if (!PlayerUnit || !PlayerUnit->HasAuthority())
		return;

	if (USpPlayerCommandComponent* CommandComponent = PlayerUnit->GetPlayerCommandComponent())
		CommandComponent->HandleCommand_Server(CommandInput);
}

void USpAbilitySystemComponent::ServerUpdatePlayerCommand_Implementation(const FSpPlayerCommandInput CommandInput)
{
	if (CommandInput.CommandId == 0 || CommandInput.Phase != ESpPlayerCommandInputPhase::Update)
		return;

	ASpPlayerUnit* PlayerUnit = Cast<ASpPlayerUnit>(GetAvatarActor());
	if (!PlayerUnit || !PlayerUnit->HasAuthority())
		return;

	if (USpPlayerCommandComponent* CommandComponent = PlayerUnit->GetPlayerCommandComponent())
		CommandComponent->HandleCommand_Server(CommandInput);
}

void USpAbilitySystemComponent::ServerConfirmPredictedAbilityForCommand_Implementation(const FGameplayAbilitySpecHandle AbilityHandle, const FPredictionKey ActivationPredictionKey, const FGameplayTag AbilityTag, const uint16 CommandId)
{
	if (!AbilityTag.IsValid() || CommandId == 0 || !AbilityHandle.IsValid())
		return;

	const FGameplayAbilitySpec* ExpectedAbilitySpec = FindAbilitySpecByTag(AbilityTag);
	if (!ExpectedAbilitySpec || ExpectedAbilitySpec->Handle != AbilityHandle)
		return;

	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!AbilitySpec || !AbilitySpec->IsActive())
		return;
	
	UGameplayAbility* AbilityInstance = AbilitySpec->GetPrimaryInstance();
	if (!AbilityInstance || AbilityInstance->GetCurrentActivationInfo().GetActivationPredictionKey() != ActivationPredictionKey)
		return;

	ASpPlayerUnit* PlayerUnit = Cast<ASpPlayerUnit>(GetAvatarActor());
	if (!PlayerUnit || !PlayerUnit->HasAuthority())
		return;

	USpPlayerCommandComponent* CommandComponent = PlayerUnit->GetPlayerCommandComponent();
	if (!CommandComponent)
		return;

	if (!CommandComponent->ValidatePredictedAbilityForCommand_Server(CommandId, AbilityTag))
		CancelAbilityHandle(AbilityHandle);
}

// gameplay event

void USpAbilitySystemComponent::SendReplicatedGameplayEvent_Client(const FGameplayAbilitySpecHandle& AbilityHandle, const FPredictionKey& ActivationPredictionKey, const FGameplayEventData& Payload)
{
	if (!AbilityHandle.IsValid() || !Payload.EventTag.IsValid() || IsOwnerActorAuthoritative())
		return;

	FScopedPredictionWindow PredictionWindow(this, true);
	ServerSendReplicatedGameplayEvent(AbilityHandle, ActivationPredictionKey, Payload, ScopedPredictionKey);
}

void USpAbilitySystemComponent::ServerSendReplicatedGameplayEvent_Implementation(FGameplayAbilitySpecHandle AbilityHandle, FPredictionKey ActivationPredictionKey, FGameplayEventData Payload, FPredictionKey PredictionKey)
{
	if (!AbilityHandle.IsValid() || !Payload.EventTag.IsValid())
		return;

	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromHandle(AbilityHandle);
	if (!AbilitySpec || !AbilitySpec->IsActive())
		return;

	FScopedPredictionWindow PredictionWindow(this, PredictionKey, true);
	OnReplicatedGameplayEvent.Broadcast(AbilityHandle, ActivationPredictionKey, Payload);
}
