#include "SpPlayerState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "ProjectSP/GameFramework/SpGameState.h"

// ==================================================

ASpPlayerState::ASpPlayerState()
{
}

void ASpPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASpPlayerState, SelectedUnitDefinition);
	DOREPLIFETIME(ASpPlayerState, bReadyToStart);
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

void ASpPlayerState::SetSelectedUnitDefinition_Server(USpUnitDefinition* UnitDefinition)
{
	check(HasAuthority());
	
	const ASpGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpGameState>() : nullptr;
	if (!GameState || !GameState->IsPlayableUnitDefinition(UnitDefinition) || GetUnitDefinition() == UnitDefinition)
		return;

	SelectedUnitDefinition = UnitDefinition;
	OnRep_SelectedUnitDefinition();
	ForceNetUpdate();
}

USpUnitDefinition* ASpPlayerState::GetUnitDefinition() const
{
	if (SelectedUnitDefinition)
		return SelectedUnitDefinition.Get();

	const ASpGameState* GameState = GetWorld() ? GetWorld()->GetGameState<ASpGameState>() : nullptr;
	return GameState ? GameState->GetDefaultUnitDefinition() : nullptr;
}

// private

void ASpPlayerState::NotifyLobbyStateChanged()
{
	if (UWorld* World = GetWorld())
	{
		if (ASpGameState* GameState = World->GetGameState<ASpGameState>())
			GameState->NotifyLobbyStateChanged();
	}
}

void ASpPlayerState::OnRep_ReadyToStart()
{
	NotifyLobbyStateChanged();
}

void ASpPlayerState::OnRep_SelectedUnitDefinition()
{
	OnSelectedUnitChanged.Broadcast();
	NotifyLobbyStateChanged();
}
