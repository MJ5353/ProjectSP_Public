#include "SpPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

ASpPlayerState::ASpPlayerState()
{
	// SetNetUpdateFrequency(100.0f);
	
	AbilitySystemComponent = CreateDefaultSubobject<USpAbilitySystemComponent>(TEXT("SpAbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void ASpPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpPlayerState, bReadyToStart);
}

void ASpPlayerState::InitializeAbilitySystem(ASpPlayerUnit* InAvatar)
{
	if (!AbilitySystemComponent || !InAvatar)
		return;

	// PlayerState는 Owner, 현재 조종 중인 플레이어 유닛은 Avatar이다. 재빙의 시 Avatar만 교체한다.
	AbilitySystemComponent->InitAbilityActorInfo(this, InAvatar);

	if (!HasAuthority() || !UnitDefinition || GrantedAbilityUnitDefinition == UnitDefinition)
		return;

	AbilitySystemComponent->SetUp(UnitDefinition);
	GrantedAbilityUnitDefinition = UnitDefinition;
}

void ASpPlayerState::SetReadyToStart_Server(const bool bInReadyToStart)
{
	check(HasAuthority());

	if (bReadyToStart == bInReadyToStart)
		return;

	bReadyToStart = bInReadyToStart;
	NotifyLobbyStateChanged();
	ForceNetUpdate();
}

// get

UAbilitySystemComponent* ASpPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

USpAbilitySystemComponent* ASpPlayerState::GetSpAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

// on rep

void ASpPlayerState::NotifyLobbyStateChanged()
{
	if (ASpGameState* GameState = GetWorld()->GetGameState<ASpGameState>())
		GameState->NotifyLobbyStateChanged();
}

void ASpPlayerState::OnRep_ReadyToStart()
{
	NotifyLobbyStateChanged();
}
