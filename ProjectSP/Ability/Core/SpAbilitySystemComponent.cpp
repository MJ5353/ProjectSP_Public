#include "SpAbilitySystemComponent.h"
#include "SpGameplayAbility.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/Animation/SpAnimInstance.h"
#include "ProjectSP/Attribute/SpHPAttributeSet.h"
#include "ProjectSP/Definition/GameplayAbility/SpAbilityDefinition.h"
#include "ProjectSP/Definition/GameplayAbility/SpAttributeDefinition.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/GameplayAbility/SpAbilityExtensionDefinition.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/Ability/Extensions/SpAbilityExtensionComponent.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

void USpAbilitySystemComponent::SetUp(const USpUnitDefinition* UnitDefinition)
{
	if (!IsOwnerActorAuthoritative() || !UnitDefinition)
		return;

	Clear();
	SetUpAbilityAndAttribute(*UnitDefinition);
	TryActivateOnSpawnAbilities();
	
	bUnitSetupComplete = true;
}

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

void USpAbilitySystemComponent::SetUpAbilityExtension(USpAbilityExtensionDefinition* InDefinition, const FGameplayTagContainer& Extensions)
{
	if (!IsOwnerActorAuthoritative())
		return;

	const bool bDefinitionChanged = ExtensionDefinition != InDefinition;
	ExtensionDefinition = InDefinition;

	FGameplayTagContainer ApplicableExtensions;
	for (const FGameplayTag& ExtensionTag : Extensions)
	{
		const FGameplayTag AbilityKey = FindAbilityKeyByExtension(ExtensionTag);
		if (!AbilityKey.IsValid() || !CanEquipAbilityExtension(AbilityKey, ExtensionTag))
			continue;

		ApplicableExtensions.AddTag(ExtensionTag);
	}

	// equip 내용이 바뀌면 force net update
	if (bDefinitionChanged || !EquippedAbilityExtensions.HasAllExact(ApplicableExtensions) || !ApplicableExtensions.HasAllExact(EquippedAbilityExtensions))
	{
		EquippedAbilityExtensions = MoveTemp(ApplicableExtensions);
		
		if (AActor* Owner = GetOwnerActor())
			Owner->ForceNetUpdate();
	}
}

void USpAbilitySystemComponent::Clear()
{
	if (IsOwnerActorAuthoritative())
	{
		bUnitSetupComplete = false;
		GrantedAbilityHandles.TakeFromAbilitySystem(this);
		GrantedAttributeSets.TakeFromAbilitySystem(this);
		EquippedAbilityExtensions.Reset();
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
		
		if (AbilityCDO && AbilityCDO->ActivationPolicy == EMjAbilityActivationPolicy::OnSpawn)
			TryActivateAbility(AbilitySpec.Handle);
	}
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

void USpAbilitySystemComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(USpAbilitySystemComponent, EquippedAbilityExtensions);
	DOREPLIFETIME(USpAbilitySystemComponent, ExtensionDefinition);
}

void USpAbilitySystemComponent::OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnRemoveAbility(AbilitySpec);

	if (!IsOwnerActorAuthoritative())
		return;

	TArray<FGameplayTag> TagsToRemove;
	for (const FGameplayTag& ExtensionTag : EquippedAbilityExtensions)
	{
		const FGameplayTag AbilityKey = FindAbilityKeyByExtension(ExtensionTag);
		if (AbilityKey.IsValid() && AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(AbilityKey))
			TagsToRemove.Add(ExtensionTag);
	}

	for (const FGameplayTag& ExtensionTag : TagsToRemove)
		EquippedAbilityExtensions.RemoveTag(ExtensionTag);

	if (!TagsToRemove.IsEmpty())
	{
		if (AActor* Owner = GetOwnerActor())
			Owner->ForceNetUpdate();
	}
}

// reset

void USpAbilitySystemComponent::ResetForDead()
{
	FGameplayTagContainer AbilityPersistTags;
	AbilityPersistTags.AddTag(SpGameplayTags::AbilityBehaviorTag_Persist);

	// Persist 태그가 없는 활성 Ability는 모두 취소
	CancelAbilities(nullptr, &AbilityPersistTags, nullptr);

	if (!IsOwnerActorAuthoritative())
		return;

	FGameplayTagContainer EffectPersistTags;
	EffectPersistTags.AddTag(SpGameplayTags::EffectBehaviorTag_Persist);

	FGameplayEffectQuery Query;
	Query.EffectTagQuery = FGameplayTagQuery::MakeQuery_MatchNoTags(EffectPersistTags);
	
	// Gameplay Effect의 Asset Tags에 Persist 태그가 없는 Effect는 모두 제거
	RemoveActiveEffects(Query);
}

void USpAbilitySystemComponent::ResetForPool()
{
	bUnitSetupComplete = false;
	CancelAllAbilities();

	if (IsOwnerActorAuthoritative())
		RemoveActiveEffects(FGameplayEffectQuery());

	bOnSpawnAbilitiesActivated = false;
}

// unit

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

	if (ASpPlayerUnit* PlayerUnit = Cast<ASpPlayerUnit>(Unit))
	{
		if (ASpPlayerController* Controller = PlayerUnit->GetController<ASpPlayerController>())
			Controller->SyncAbilityExtensions_Server(false);
	}
}

// attribute ------------------------------------------------

void USpAbilitySystemComponent::UnregisterAttributeChangeCallback(const FGameplayAttribute& Attribute, FDelegateHandle& Handle)
{
	if (!Attribute.IsValid() || !Handle.IsValid())
		return;

	FOnGameplayAttributeValueChange& ChangeDelegate = GetGameplayAttributeValueChangeDelegate(Attribute);
	ChangeDelegate.Remove(Handle);
	Handle.Reset();
}

void USpAbilitySystemComponent::NotifyDamageApplied(AActor* Instigator, float AppliedDamage)
{
	if (!IsOwnerActorAuthoritative() || AppliedDamage <= 0.0f)
		return;

	OnDamageApplied.Broadcast(Instigator, AppliedDamage);
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

// ability tag ------------------------------------------------

bool USpAbilitySystemComponent::IsAbilityTagActive(FGameplayTag Tag) const
{
	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(Tag);
	if (!AbilitySpec || !AbilitySpec->Ability)
		return false;

	return AbilitySpec->IsActive();
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

// extension ------------------------------------------------

bool USpAbilitySystemComponent::CanEquipAbilityExtension(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag) const
{
	if (!IsAbilityExtensionTag(AbilityKey, ExtensionTag))
		return false;

	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(AbilityKey);
	if (!AbilitySpec || !AbilitySpec->Ability)
		return false;

	const EGameplayAbilityInstancingPolicy::Type Policy = AbilitySpec->Ability->GetInstancingPolicy();
	return Policy == EGameplayAbilityInstancingPolicy::InstancedPerActor || Policy == EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

bool USpAbilitySystemComponent::IsAbilityExtensionTag(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag) const
{
	return ExtensionDefinition && ExtensionDefinition->FindOption(AbilityKey, ExtensionTag);
}

FGameplayTag USpAbilitySystemComponent::FindAbilityKeyByExtension(const FGameplayTag ExtensionTag) const
{
	return ExtensionDefinition ? ExtensionDefinition->FindAbilityKeyByExtension(ExtensionTag) : FGameplayTag();
}

bool USpAbilitySystemComponent::AcquireAbilityExtension(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag)
{
	if (!IsOwnerActorAuthoritative())
		return false;

	if (!CanEquipAbilityExtension(AbilityKey, ExtensionTag))
		return false;

	if (ASpPlayerUnit* PlayerUnit = Cast<ASpPlayerUnit>(GetOwnerActor()))
	{
		ASpPlayerController* PC = PlayerUnit->GetController<ASpPlayerController>();
		USpAbilityExtensionComponent* Extensions = PC ? PC->GetAbilityExtensionComponent() : nullptr;
		
		return Extensions && Extensions->AcquireExtension_Server(AbilityKey, ExtensionTag);
	}

	if (EquippedAbilityExtensions.HasTagExact(ExtensionTag))
		return false;

	EquippedAbilityExtensions.AddTag(ExtensionTag);

	// sync
	if (AActor* Owner = GetOwnerActor())
		Owner->ForceNetUpdate();
	
	return true;
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

bool USpAbilitySystemComponent::HasEquippedAbilityExtension(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag) const
{
	return IsAbilityExtensionTag(AbilityKey, ExtensionTag) && FindAbilitySpecByTag(AbilityKey) && EquippedAbilityExtensions.HasTagExact(ExtensionTag);
}

bool USpAbilitySystemComponent::TryGetAbilityExecuteRangeByTag(FGameplayTag AbilityTag, float& OutExecuteRange) const
{
	OutExecuteRange = 0.0f;

	const FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecByTag(AbilityTag);
	const USpGameplayAbility* Ability = AbilitySpec ? Cast<USpGameplayAbility>(AbilitySpec->Ability) : nullptr;
	
	if (!Ability)
		return false;

	OutExecuteRange = FMath::Max(Ability->ExecuteRange, 0.0f);
	return true;
}

// gameplay event ------------------------------------------------

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
