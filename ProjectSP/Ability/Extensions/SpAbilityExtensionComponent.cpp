#include "SpAbilityExtensionComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Common/SpLog.h"
#include "ProjectSP/Definition/GameplayAbility/SpAbilityExtensionDefinition.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/GameFramework/SpPlayerController.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

USpAbilityExtensionComponent::USpAbilityExtensionComponent()
{
	SetIsReplicatedByDefault(true);
}

void USpAbilityExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(USpAbilityExtensionComponent, ActiveState, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(USpAbilityExtensionComponent, ExtensionPoints, COND_OwnerOnly);
}

// unit sync

void USpAbilityExtensionComponent::SetUp_Server(const USpUnitDefinition* UnitDefinition, const bool bPawnChanged)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	USpUnitDefinition* InUnitDefinition = IsValid(UnitDefinition) ? const_cast<USpUnitDefinition*>(UnitDefinition) : nullptr;
	USpAbilityExtensionDefinition* InExtensionDefinition = InUnitDefinition ? InUnitDefinition->AbilityExtensionDefinition.Get() : nullptr;
	FGameplayTagContainer ActiveExtensions;

	if (IsValid(InExtensionDefinition))
	{
		FGameplayTagContainer& UnlockedExtensions = UnlockedExtensionsByUnit.FindOrAdd(InUnitDefinition);
		for (const FGameplayTag& ExtensionTag : InUnitDefinition->InitiallyEquippedExtensions)
		{
			if (!InExtensionDefinition->FindOptionByExtension(ExtensionTag))
			{
				UE_LOG(LogMj, Warning, TEXT("Unit initial extension tag is missing or ambiguous: %s"), *ExtensionTag.ToString());
				continue;
			}

			UnlockedExtensions.AddTag(ExtensionTag);
		}

		for (const FGameplayTag& ExtensionTag : UnlockedExtensions)
		{
			if (InExtensionDefinition->FindAbilityKeyByExtension(ExtensionTag).IsValid())
				ActiveExtensions.AddTag(ExtensionTag);
		}
	}

	const bool bDefinitionChanged = ActiveState.UnitDefinition.Get() != InUnitDefinition || ActiveState.ExtensionDefinition.Get() != InExtensionDefinition;
	const bool bExtensionChanged = !ActiveState.EquippedExtensions.HasAllExact(ActiveExtensions) || !ActiveExtensions.HasAllExact(ActiveState.EquippedExtensions);
	const bool bStateChanged = bDefinitionChanged || bExtensionChanged;

	if (bStateChanged)
	{
		ActiveState.UnitDefinition = InUnitDefinition;
		ActiveState.ExtensionDefinition = InExtensionDefinition;
		ActiveState.EquippedExtensions = MoveTemp(ActiveExtensions);
		RefreshPublishedState();
	}

	ApplyToASC_Server();

	if (bPawnChanged || bStateChanged)
		NotifyStateChanged_Server();
}

// extension change

ESpAbilityExtensionPurchaseResult USpAbilityExtensionComponent::TryPurchaseExtension_Server(const FGameplayTag ExtensionTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return ESpAbilityExtensionPurchaseResult::CannotEquip;

	const UWorld* World = GetWorld();
	const ASpGameState* GameState = World ? World->GetGameState<ASpGameState>() : nullptr;
	
	if (!GameState || !GameState->IsGameplayActive())
		return ESpAbilityExtensionPurchaseResult::NotPlaying;

	USpUnitDefinition* UnitDefinition = ActiveState.UnitDefinition.Get();
	USpAbilityExtensionDefinition* ExtensionDefinition = ActiveState.ExtensionDefinition.Get();
	
	if (!IsValid(UnitDefinition) || !IsValid(ExtensionDefinition))
		return ESpAbilityExtensionPurchaseResult::NoActiveUnit;

	const FGameplayTag AbilityKey = ExtensionDefinition->FindAbilityKeyByExtension(ExtensionTag);
	if (!IsConfigured(AbilityKey, ExtensionTag))
		return ESpAbilityExtensionPurchaseResult::InvalidExtension;

	const FGameplayTagContainer* UnlockedExtensions = UnlockedExtensionsByUnit.Find(UnitDefinition);
	if (ActiveState.EquippedExtensions.HasTagExact(ExtensionTag) || (UnlockedExtensions && UnlockedExtensions->HasTagExact(ExtensionTag)))
		return ESpAbilityExtensionPurchaseResult::AlreadyUnlocked;

	if (ExtensionPoints < 1)
		return ESpAbilityExtensionPurchaseResult::InsufficientPoints;

	USpAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
		return ESpAbilityExtensionPurchaseResult::NoActiveUnit;

	if (!ASC->HasCompletedUnitSetup() || !ASC->CanEquipAbilityExtension(AbilityKey, ExtensionTag))
		return ESpAbilityExtensionPurchaseResult::CannotEquip;

	ActiveState.EquippedExtensions.AddTag(ExtensionTag);
	ASC->SetUpAbilityExtension(ExtensionDefinition, ActiveState.EquippedExtensions);
	
	if (!ASC->HasEquippedAbilityExtension(AbilityKey, ExtensionTag))
	{
		ActiveState.EquippedExtensions.RemoveTag(ExtensionTag);
		ASC->SetUpAbilityExtension(ExtensionDefinition, ActiveState.EquippedExtensions);
		
		return ESpAbilityExtensionPurchaseResult::CannotEquip;
	}

	UnlockedExtensionsByUnit.FindOrAdd(UnitDefinition).AddTag(ExtensionTag);
	--ExtensionPoints; // 포인트 차감
	
	RefreshPublishedState();
	NotifyStateChanged_Server();

	OnExtensionPointsChanged.Broadcast(ExtensionPoints);
	return ESpAbilityExtensionPurchaseResult::Success;
}

void USpAbilityExtensionComponent::GrantExtensionPoint_Server()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || ExtensionPoints == MAX_int32)
		return;

	++ExtensionPoints;
	OnExtensionPointsChanged.Broadcast(ExtensionPoints);

	GetOwner()->ForceNetUpdate();
}

bool USpAbilityExtensionComponent::AcquireExtension_Server(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag)
{
	return AddEquippedExtension_Server(AbilityKey, ExtensionTag);
}

bool USpAbilityExtensionComponent::AddEquippedExtension_Server(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag)
{
	USpUnitDefinition* UnitDefinition = ActiveState.UnitDefinition.Get();
	if (!GetOwner() || !GetOwner()->HasAuthority() || !UnitDefinition || !IsConfigured(AbilityKey, ExtensionTag) || ActiveState.EquippedExtensions.HasTagExact(ExtensionTag))
		return false;

	USpAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC || !ASC->CanEquipAbilityExtension(AbilityKey, ExtensionTag))
		return false;

	UnlockedExtensionsByUnit.FindOrAdd(UnitDefinition).AddTag(ExtensionTag);
	ActiveState.EquippedExtensions.AddTag(ExtensionTag);
	
	RefreshPublishedState();
	ApplyToASC_Server();
	NotifyStateChanged_Server();

	return true;
}

void USpAbilityExtensionComponent::ApplyToASC_Server()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (USpAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (ASC->HasCompletedUnitSetup())
			ASC->SetUpAbilityExtension(ActiveState.ExtensionDefinition.Get(), ActiveState.EquippedExtensions);
	}
}

void USpAbilityExtensionComponent::RefreshPublishedState()
{
	Definition = ActiveState.ExtensionDefinition;
	EquippedExtensions = ActiveState.EquippedExtensions;
}

void USpAbilityExtensionComponent::NotifyStateChanged_Server()
{
	K2_OnExtensionStateChanged();
	GetOwner()->ForceNetUpdate();
}

// rpc

void USpAbilityExtensionComponent::OnRep_ActiveState()
{
	RefreshPublishedState();
	K2_OnExtensionStateChanged();
}

void USpAbilityExtensionComponent::OnRep_ExtensionPoints()
{
	OnExtensionPointsChanged.Broadcast(ExtensionPoints);
}

// get

bool USpAbilityExtensionComponent::IsConfigured(const FGameplayTag AbilityKey, const FGameplayTag ExtensionTag) const
{
	return ActiveState.ExtensionDefinition && ActiveState.ExtensionDefinition->FindOption(AbilityKey, ExtensionTag);
}

USpAbilitySystemComponent* USpAbilityExtensionComponent::GetAbilitySystemComponent() const
{
	const ASpPlayerController* Controller = Cast<ASpPlayerController>(GetOwner());
	const ASpPlayerUnit* Unit = Controller ? Controller->GetPawn<ASpPlayerUnit>() : nullptr;
	const USpUnitDefinition* UnitDefinition = IsValid(Unit) ? Unit->GetUnitDefinition() : nullptr;

	if (!IsValid(UnitDefinition) || ActiveState.UnitDefinition.Get() != UnitDefinition || ActiveState.ExtensionDefinition.Get() != UnitDefinition->AbilityExtensionDefinition.Get())
		return nullptr;

	return Unit->GetSpAbilitySystemComponent();
}
