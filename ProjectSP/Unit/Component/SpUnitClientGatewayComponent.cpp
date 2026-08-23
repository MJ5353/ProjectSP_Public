#include "SpUnitClientGatewayComponent.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Interface/SpUnitClientListener.h"

// ==================================================

void USpUnitClientGatewayComponent::InitializePresentation_ClientOnly()
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	bPresentationInitialized = true;
	Unit->SetActorHiddenInGame(true);

	ApplyUnitData_ClientOnly(Unit->GetUnitData());
}

void USpUnitClientGatewayComponent::ApplyUnitData_ClientOnly(const FSpUnitData& UnitData)
{
	if (!bPresentationInitialized)
		return;

	ASpUnit* Unit = GetOwnerUnitChecked();
	Unit->ApplyReplicatedUnitData_ClientOnly();

	if (!UnitData.IsValid())
	{
		StopPresentation_ClientOnly();
		return;
	}

	if (!NotifyPresentationPrepared_ClientOnly(UnitData))
	{
		Unit->SetActorHiddenInGame(true);
		return;
	}

	bPresentationStarted = true;
	Unit->SetActorHiddenInGame(false);

	if (UWorld* World = Unit->GetWorld())
	{
		if (ASpGameState* GameState = World->GetGameState<ASpGameState>())
			GameState->NotifyUnitPresentationReady_ClientOnly(UnitData.UnitUid);
	}
}

bool USpUnitClientGatewayComponent::NotifyPresentationPrepared_ClientOnly(const FSpUnitData& UnitData) const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	TInlineComponentArray<UActorComponent*> Components(Unit);

	// 클라이언트 전용 comp에 전달
	for (UActorComponent* Component : Components)
	{
		if (ISpUnitClientListener* Listener = Cast<ISpUnitClientListener>(Component))
		{
			// 하나라도 실패하면 false - 나중에 전체 성공시 prepare로 처리하고 싶으면 사용
			if (!Listener->PrepareClientPresentation(UnitData))
				return false;
		}
	}
	
	return true;
}

void USpUnitClientGatewayComponent::StopPresentation_ClientOnly()
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	Unit->SetActorHiddenInGame(true);

	if (bPresentationStarted)
	{
		TInlineComponentArray<UActorComponent*> Components(Unit);
		for (UActorComponent* Component : Components)
		{
			if (ISpUnitClientListener* Listener = Cast<ISpUnitClientListener>(Component))
				Listener->StopClientPresentation();
		}
	}

	bPresentationStarted = false;
}

// get

ASpUnit* USpUnitClientGatewayComponent::GetOwnerUnitChecked() const
{
	ASpUnit* Unit = GetOwner<ASpUnit>();
	
	check(Unit);
	check(Unit->GetNetMode() != NM_DedicatedServer);
	
	return Unit;
}
