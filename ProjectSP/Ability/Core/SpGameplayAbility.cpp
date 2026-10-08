#include "SpGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
#include "ProjectSP/Ability/Core/SpCooldownGameplayEffect.h"
#include "ProjectSP/Ability/Extensions/SpAbilityModifierBase.h"
#include "ProjectSP/Common/SpLog.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

namespace
{
	FGameplayEffectQuery MakeSharedCooldownQuery(const FGameplayTag AbilityKey)
	{
		FGameplayEffectQuery Query;
		Query.CustomMatchDelegate.BindLambda([AbilityKey](const FActiveGameplayEffect& ActiveEffect)
		{
			return ActiveEffect.Spec.Def && ActiveEffect.Spec.Def->IsA(USpCooldownGameplayEffect::StaticClass())
				&& ActiveEffect.Spec.DynamicGrantedTags.HasTagExact(AbilityKey);
		});
		return Query;
	}
}

// hook 함수 안에서 lifecycle active나 is active가 바뀔 수 있으므로
#define SP_EXECUTE_ACTIVE_MODIFIER_HOOK(HookName) \
	do \
	{ \
		for (USpAbilityModifierBase* Modifier : Modifiers) \
		{ \
			if (!bModifierLifecycleActive || !IsActive()) \
				break; \
			if (IsValid(Modifier)) \
				Modifier->HookName(this); \
		} \
	} while (false)

// ==================================================

bool USpGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
		return false;

	const bool bNeedTarget = GetAssetTags().HasTagExact(SpGameplayTags::AbilityTargetTag_Hostile);
	
	if (!bNeedTarget)
		return true;

	const ASpUnit* Unit = Cast<ASpUnit>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (!Unit)
		return false;

	const ASpUnit* Target = Unit->GetTargetActor();
	if (!Unit->IsAttackable(Target))
		return false;
	
	if (!Unit->IsA<ASpPlayerUnit>())
		return true;

	if (FVector::DistSquared2D(Unit->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(FMath::Max(ExecuteRange, 0.0f)))
		return false;

	return !GetAssetTags().HasTagExact(SpGameplayTags::AbilityConditionTag_TargetFacing) || Unit->IsFacingTargetLocation(Target->GetActorLocation());
}

bool USpGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	if (!Super::CommitAbility(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags))
		return false;

	SP_EXECUTE_ACTIVE_MODIFIER_HOOK(OnCommitAbility);
	return true;
}

void USpGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (IsInstantiated())
	{
		bModifierLifecycleActive = !Modifiers.IsEmpty();
		bDispatchingModifierEnd = false;
		ActivationTarget.Reset();
		
		if (GetAssetTags().HasTagExact(SpGameplayTags::AbilityTargetTag_Hostile))
		{
			AActor* AvatarActor = GetAvatarActorFromActorInfo();
			if (const ASpUnit* Unit = Cast<ASpUnit>(AvatarActor))
			{
				ActivationTarget = Unit->GetTargetActor();
			}
		}

		SP_EXECUTE_ACTIVE_MODIFIER_HOOK(OnActivateAbility);

		if (!IsActive())
			return;
	}
	else if (!Modifiers.IsEmpty())
	{
		UE_LOG(LogMj, Warning, TEXT("[mj] AbilityModifier requires an instanced ability: %s"), *GetName());
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void USpGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const bool bReplicateEndAbility, const bool bWasCancelled)
{
	if (bDispatchingModifierEnd)
		return;

	bDispatchingModifierEnd = true;
	if (bModifierLifecycleActive && ScopeLockCount == 0 && IsEndAbilityValid(Handle, ActorInfo))
	{
		bModifierLifecycleActive = false;
		for (USpAbilityModifierBase* Modifier : Modifiers)
		{
			if (!IsValid(Modifier))
				continue;

			if (bWasCancelled)
				Modifier->OnCancelAbility(this);

			if (IsValid(Modifier))
				Modifier->OnEndAbility(this);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	if (IsInstantiated())
		ActivationTarget.Reset();

	bDispatchingModifierEnd = false;
}

ESpAbilityTagBranchResult USpGameplayAbility::HasTagBranch(const FGameplayTag Tag) const
{
	if (!Tag.IsValid())
		return ESpAbilityTagBranchResult::Failure;

	if (IsInstantiated())
	{
		if (const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec())
		{
			const USpAbilitySystemComponent* ASC = Cast<USpAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
			const FGameplayTag AbilityKey = ASC ? ASC->FindAbilityKeyByExtension(Tag) : FGameplayTag();
			
			if (AbilityKey.IsValid() && Spec->GetDynamicSpecSourceTags().HasTagExact(AbilityKey))
				return ASC->HasEquippedAbilityExtension(AbilityKey, Tag) ? ESpAbilityTagBranchResult::Success : ESpAbilityTagBranchResult::Failure;
		}
	}

	return GetAssetTags().HasTagExact(Tag) ? ESpAbilityTagBranchResult::Success : ESpAbilityTagBranchResult::Failure;
}

void USpGameplayAbility::ApplyAttackDamageToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> DamageEffectClass)
{
	if (!DamageEffectClass || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority())
		return;

	if (!IsValid(TargetActor) && IsInstantiated())
		TargetActor = ActivationTarget.Get();
	
	if (!IsValid(TargetActor))
		return;
	
	ASpUnit* AttackerUnit = Cast<ASpUnit>(GetAvatarActorFromActorInfo());
	ASpUnit* TargetUnit = Cast<ASpUnit>(TargetActor);
	
	if (!AttackerUnit || !TargetUnit || !AttackerUnit->IsAttackable(TargetUnit))
		return;

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	if (!SourceASC || !TargetASC)
		return;
	
	FGameplayEffectSpecHandle DamageSpec = MakeOutgoingGameplayEffectSpec(DamageEffectClass, GetAbilityLevel());
	if (!DamageSpec.IsValid())
		return;
	
	// GE_Damage가 공격자의 AttackPower를 평가해 대상의 Damage에 적용한다.
	const FActiveGameplayEffectHandle AppliedDamageHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
}

// void USpGameplayAbility::SendHitReactionEvent(AActor* TargetActor, const FGameplayEffectSpecHandle& DamageSpec)
// {
	// FGameplayEventData HitEventData;
	// HitEventData.EventTag = SpGameplayTags::UnitReactionTag_Hit;
	// HitEventData.Instigator = GetAvatarActorFromActorInfo();
	// HitEventData.Target = TargetActor;
	// HitEventData.ContextHandle = DamageSpec.Data->GetContext();
	//
	// UE_LOG(LogMj, Log, TEXT("[mj] SendHitReactionEvent : %s -> %s"), *GetAvatarActorFromActorInfo()->GetName(), *TargetActor->GetName());
	// UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, HitEventData.EventTag, HitEventData);
// }

// cooldown

void USpGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	if (CooldownDuration <= 0.0f)
	{
		Super::ApplyCooldown(Handle, ActorInfo, ActivationInfo);
		return;
	}

	const FGameplayTag AbilityKey = GetCooldownAbilityKey(Handle, ActorInfo);
	if (!AbilityKey.IsValid())
		return;

	const UGameplayEffect* CooldownGE = GetCooldownGameplayEffect();
	if (!CooldownGE)
		return;

	FGameplayEffectSpecHandle CooldownSpec = MakeOutgoingGameplayEffectSpec(Handle, ActorInfo, ActivationInfo, CooldownGE->GetClass(), GetAbilityLevel(Handle, ActorInfo));
	if (!CooldownSpec.IsValid())
		return;

	CooldownSpec.Data->SetSetByCallerMagnitude(USpCooldownGameplayEffect::GetDurationSetByCallerTag(), CooldownDuration);
	CooldownSpec.Data->DynamicGrantedTags.AddTag(AbilityKey);
	ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, CooldownSpec);
}

bool USpGameplayAbility::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (CooldownDuration <= 0.0f)
		return Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags);

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTag AbilityKey = GetCooldownAbilityKey(Handle, ActorInfo);
	
	if (!ASC || !AbilityKey.IsValid())
		return false;

	if (ASC->GetActiveEffectsTimeRemaining(MakeSharedCooldownQuery(AbilityKey)).IsEmpty())
		return true;

	if (OptionalRelevantTags)
	{
		const FGameplayTag& FailCooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;
		if (FailCooldownTag.IsValid())
			OptionalRelevantTags->AddTag(FailCooldownTag);
		
		OptionalRelevantTags->AddTag(AbilityKey);
	}

	return false;
}

float USpGameplayAbility::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownDuration <= 0.0f)
		return Super::GetCooldownTimeRemaining(ActorInfo);

	float TimeRemaining = 0.0f;
	float Duration = 0.0f;

	GetCooldownTimeRemainingAndDuration(CurrentSpecHandle, ActorInfo, TimeRemaining, Duration);
	return TimeRemaining;
}

void USpGameplayAbility::GetCooldownTimeRemainingAndDuration(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, float& TimeRemaining, float& OutCooldownDuration) const
{
	if (CooldownDuration <= 0.0f)
	{
		Super::GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, TimeRemaining, OutCooldownDuration);
		return;
	}

	TimeRemaining = 0.0f;
	OutCooldownDuration = 0.0f;

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayTag AbilityKey = GetCooldownAbilityKey(Handle, ActorInfo);
	
	if (!ASC || !AbilityKey.IsValid())
		return;

	const TArray<TPair<float, float>> ActiveCooldowns = ASC->GetActiveEffectsTimeRemainingAndDuration(MakeSharedCooldownQuery(AbilityKey));
	for (const TPair<float, float>& Cooldown : ActiveCooldowns)
	{
		if (Cooldown.Key > TimeRemaining)
		{
			TimeRemaining = Cooldown.Key;
			OutCooldownDuration = Cooldown.Value;
		}
	}
}

UGameplayEffect* USpGameplayAbility::GetCooldownGameplayEffect() const
{
	UGameplayEffect* ConfiguredGE = Super::GetCooldownGameplayEffect();
	if (CooldownDuration <= 0.0f || (ConfiguredGE && ConfiguredGE->IsA(USpCooldownGameplayEffect::StaticClass())))
		return ConfiguredGE;

	return USpCooldownGameplayEffect::StaticClass()->GetDefaultObject<UGameplayEffect>();
}

FGameplayTag USpGameplayAbility::GetCooldownAbilityKey(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	const FGameplayAbilitySpec* AbilitySpec = ASC ? ASC->FindAbilitySpecFromHandle(Handle) : nullptr;

	if (!AbilitySpec)
		return FGameplayTag();

	const FGameplayTag AbilityKeyRoot = SpGameplayTags::AbilityKeyTag;
	for (const FGameplayTag& Tag : AbilitySpec->GetDynamicSpecSourceTags())
	{
		if (Tag.MatchesTag(AbilityKeyRoot) && Tag != AbilityKeyRoot)
			return Tag;
	}

	return FGameplayTag();
}

#undef SP_EXECUTE_ACTIVE_MODIFIER_HOOK
