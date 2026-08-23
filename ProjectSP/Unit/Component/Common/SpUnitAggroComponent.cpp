#include "SpUnitAggroComponent.h"
#include "Engine/World.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/Definition/Unit/AI/AggroDefinition.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Definition/Unit/AI/SpEnemyAIDefinition.h"

// ==================================================

USpUnitAggroComponent::USpUnitAggroComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USpUnitAggroComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		// Threat가 존재할 때만 켜짐
		SetComponentTickEnabled(false);
		return;
	}
	
	UpdateThreatDecay(DeltaTime);
}

// unit lifecycle

void USpUnitAggroComponent::OnInitUnit()
{
	ASpUnit* OwnerUnit = Cast<ASpUnit>(GetOwner());
	if (!OwnerUnit || !OwnerUnit->HasAuthority())
		return;

	USpAbilitySystemComponent* SpASC = OwnerUnit->GetSpAbilitySystemComponent();
	if (!SpASC || DamageAppliedHandle.IsValid())
		return;

	DamageAppliedHandle = SpASC->OnDamageApplied.AddUObject(this, &ThisClass::AddDamageThreat);
}

void USpUnitAggroComponent::OnClearUnit()
{
	if (ASpUnit* OwnerUnit = Cast<ASpUnit>(GetOwner()))
	{
		if (USpAbilitySystemComponent* SpASC = OwnerUnit->GetSpAbilitySystemComponent())
			SpASC->OnDamageApplied.Remove(DamageAppliedHandle);
	}

	DamageAppliedHandle.Reset();
	ClearThreat();
}

void USpUnitAggroComponent::OnUnitActive(bool bActive)
{
	ClearThreat();
	
	if (!bActive)
		return;
	
	SetUp();
}

// stimuli

void USpUnitAggroComponent::SetTargetSensed(AActor* Actor, bool bSensed)
{
	if (!AggroDefinition)
		return;
	
	ASpUnit* TargetUnit = Cast<ASpUnit>(Actor);
	if (!TargetUnit)
		return;

	FSpAggroEntry* ExistingEntry = ThreatEntries.Find(TargetUnit);
	if (!bSensed)
	{
		if (ExistingEntry)
			ExistingEntry->bVisible = false;

		return;
	}

	if (!IsValidThreatTarget(TargetUnit))
		return;

	const bool bNewEntry = ExistingEntry == nullptr;
	
	// 있으면 쓰고, 없으면 만들기
	FSpAggroEntry& Entry = ExistingEntry ? *ExistingEntry : ThreatEntries.Add(TargetUnit);
	const bool bWasVisible = Entry.bVisible;

	if (bNewEntry)
		Entry.Threat = FMath::Max(AggroDefinition->SightInitialThreat, 0.0f);

	// 새로 인지됨
	Entry.bVisible = true;
	Entry.LastStimulusTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	SetComponentTickEnabled(true);

	if (bNewEntry || !bWasVisible)
		BroadcastAggroChanged();
}

void USpUnitAggroComponent::AddDamageThreat(AActor* Instigator, float AppliedDamage)
{
	if (!AggroDefinition)
		return;
	
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || AppliedDamage <= 0.0f)
		return;

	ASpUnit* TargetUnit = ResolveThreatTarget(Instigator);
	if (!IsValidThreatTarget(TargetUnit))
		return;

	const float AddedThreat = AppliedDamage * FMath::Max(AggroDefinition->DamageThreatMultiplier, 0.0f);
	if (AddedThreat <= 0.0f)
		return;

	FSpAggroEntry& Entry = ThreatEntries.FindOrAdd(TargetUnit);
	Entry.Threat += AddedThreat;
	Entry.LastStimulusTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	
	SetComponentTickEnabled(true);
	BroadcastAggroChanged();
}

void USpUnitAggroComponent::ClearThreat()
{
	const bool bHadThreat = !ThreatEntries.IsEmpty();
	ThreatEntries.Reset();
	
	SetComponentTickEnabled(false);
	
	if (bHadThreat)
		BroadcastAggroChanged();
}

// set

void USpUnitAggroComponent::SetUp()
{
	ASpUnit* Unit = GetOwner<ASpUnit>();
	if (!Unit)
		return; 
	
	const USpUnitDefinition* UnitDefinition = Unit->GetUnitDefinition();
	if (!UnitDefinition || !UnitDefinition->AIDefinition)
		return;
	
	USpEnemyAIDefinition* AIDefinition = Cast<USpEnemyAIDefinition>(UnitDefinition->AIDefinition);
	if (!AIDefinition)
		return;
	
	AggroDefinition = AIDefinition->AggroDefinition;
}

// check target

bool USpUnitAggroComponent::IsValidThreatTarget(const ASpUnit* TargetUnit) const
{
	const ASpUnit* OwnerUnit = Cast<ASpUnit>(GetOwner());
	
	if (!OwnerUnit || !IsValid(TargetUnit) || !TargetUnit->IsAttackable(OwnerUnit))
		return false;
	
	ETeamAttitude::Type Attitude = TargetUnit->GetTeamAttitudeTowards(*OwnerUnit);
	return Attitude == ETeamAttitude::Hostile;
}

ASpUnit* USpUnitAggroComponent::ResolveThreatTarget(AActor* Instigator) const
{
	AActor* Candidate = Instigator;
	
	// 인지된 Actor가 projectile일 경우를 고려...
	for (int32 Depth = 0; Candidate && Depth < 8; ++Depth)
	{
		if (ASpUnit* CandidateUnit = Cast<ASpUnit>(Candidate))
		{
			if (ASpUnit* OwnerUnit = Cast<ASpUnit>(CandidateUnit->GetOwner()))
			{
				Candidate = OwnerUnit;
				continue;
			}

			return CandidateUnit;
		}
		
		Candidate = Candidate->GetOwner();
	}

	return nullptr;
}

// threat manage

void USpUnitAggroComponent::UpdateThreatDecay(float DeltaTime)
{
	if (!AggroDefinition)
		return;
	
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	const float ClampedDeltaTime = FMath::Max(DeltaTime, 0.0f);
	const float ClampedDecayPerSec = FMath::Clamp(AggroDefinition->ThreatDecayPercentPerSecond, 0.0f, 1.0f);
	const float DecayMultiplier = FMath::Pow(1.0f - ClampedDecayPerSec, ClampedDeltaTime);
	
	bool bThreatListChanged = false;
	auto It = ThreatEntries.CreateIterator();
	
	for (; It; ++It)
	{
		ASpUnit* TargetUnit = It.Key().Get();
		if (!IsValidThreatTarget(TargetUnit))
		{
			It.RemoveCurrent();
			bThreatListChanged = true;
			
			continue;
		}

		FSpAggroEntry& Entry = It.Value();
		if (!Entry.bVisible && CurrentTime - Entry.LastStimulusTime >= AggroDefinition->ThreatMemorySeconds)
		{
			Entry.Threat *= DecayMultiplier;
			if (Entry.Threat <= AggroDefinition->ThreatRemovalThreshold)
			{
				It.RemoveCurrent();
				bThreatListChanged = true;
			}
		}
	}

	if (ThreatEntries.IsEmpty())
		SetComponentTickEnabled(false);

	if (bThreatListChanged)
		BroadcastAggroChanged();
}

void USpUnitAggroComponent::BroadcastAggroChanged()
{
	OnAggroChanged.Broadcast();
}

// get

ASpUnit* USpUnitAggroComponent::GetBestTarget(AActor* CurrentTarget, const float TargetSwitchRatio, TFunctionRef<bool(const ASpUnit*)> IsTargetEligible) const
{
	ASpUnit* BestTarget = nullptr;
	float BestThreat = 0.0f;

	for (const TPair<TWeakObjectPtr<ASpUnit>, FSpAggroEntry>& Pair : ThreatEntries)
	{
		ASpUnit* Candidate = Pair.Key.Get();
		if (!Candidate || !IsValidThreatTarget(Candidate) || !IsTargetEligible(Candidate))
			continue;

		if (!BestTarget || Pair.Value.Threat > BestThreat)
		{
			BestTarget = Candidate;
			BestThreat = Pair.Value.Threat;
		}
	}

	ASpUnit* CurrentUnit = Cast<ASpUnit>(CurrentTarget);
	if (!CurrentUnit || !BestTarget || CurrentUnit == BestTarget || !IsValidThreatTarget(CurrentUnit) || !IsTargetEligible(CurrentUnit))
		return BestTarget;

	const FSpAggroEntry* CurrentEntry = ThreatEntries.Find(CurrentUnit);
	if (!CurrentEntry)
		return BestTarget;

	const float RequiredThreat = CurrentEntry->Threat * FMath::Max(TargetSwitchRatio, 1.0f);
	return BestThreat >= RequiredThreat ? BestTarget : CurrentUnit;
}
