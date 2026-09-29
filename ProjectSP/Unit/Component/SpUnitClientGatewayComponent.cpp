#include "SpUnitClientGatewayComponent.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Interface/SpUnitClientListener.h"

// ==================================================

// public

void USpUnitClientGatewayComponent::InitializePresentation_Client()
{
	if (bPresentationInitialized)
		return;

	bPresentationInitialized = true;
	
	ASpUnit* Unit = GetOwnerUnitChecked();
	Unit->SetUnitPresentationVisible_Client(false);
	
	const FSpUnitData& UnitData = Unit->GetUnitData();
	ApplyUnitData_Client(UnitData);
}

void USpUnitClientGatewayComponent::ApplyUnitData_Client(const FSpUnitData& UnitData)
{
	if (!bPresentationInitialized)
		return;

	ASpUnit* Unit = GetOwnerUnitChecked();
	Unit->ApplyReplicatedUnitData_Client();

	if (!UnitData.IsValid())
	{
		StopPresentation_Client();
		return;
	}

	if (PresentationUnitUid != UnitData.UnitUid)
	{
		StopPresentation_Client();
		BeginPresentation_Client(UnitData);
	}
}

void USpUnitClientGatewayComponent::ReportPresentationReady_Client(const UActorComponent* Listener, const uint32 UnitUid)
{
	if (!Listener || bPresentationReady || UnitUid != PresentationUnitUid)
		return;

	if (PendingPresentationListeners.Remove(Listener) > 0 && PendingPresentationListeners.IsEmpty())
		CompletePresentation_Client();
}

void USpUnitClientGatewayComponent::ReportPresentationReadyToGameState_Client()
{
	if (!bPresentationReady || PresentationUnitUid == 0)
		return;

	if (ASpGameState* GameState = GetOwnerUnitChecked()->GetWorld()->GetGameState<ASpGameState>())
		GameState->NotifyUnitPresentationReady_Client(PresentationUnitUid);
}

// private

void USpUnitClientGatewayComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bPresentationInitialized)
		StopPresentation_Client();

	Super::EndPlay(EndPlayReason);
}

void USpUnitClientGatewayComponent::BeginPresentation_Client(const FSpUnitData& UnitData)
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	PresentationUnitUid = UnitData.UnitUid;

	TInlineComponentArray<UActorComponent*> Components(Unit);
	TArray<ISpUnitClientListener*, TInlineAllocator<8>> ClientListeners;
	ClientListeners.Reserve(Components.Num());

	for (UActorComponent* Component : Components)
	{
		if (ISpUnitClientListener* Listener = Cast<ISpUnitClientListener>(Component))
		{
			ClientListeners.Add(Listener);

			if (Listener->IsClientPresentationRequired())
				PendingPresentationListeners.Add(Component);
		}
	}

	// for문을 따로 돌리는 이유는, pending list를 완성한 뒤에 prepare를 호출하기 위함
	
	for (ISpUnitClientListener* Listener : ClientListeners)
		Listener->PrepareClientPresentation(UnitData);

	if (PendingPresentationListeners.IsEmpty())
		CompletePresentation_Client();
}

void USpUnitClientGatewayComponent::CompletePresentation_Client()
{
	if (bPresentationReady || PresentationUnitUid == 0)
		return;

	bPresentationReady = true;
	ReportPresentationReadyToGameState_Client();

	ASpUnit* Unit = GetOwnerUnitChecked();
	const FSpUnitData& UnitData = Unit->GetUnitData();
	
	if (UnitData.UnitUid == PresentationUnitUid && UnitData.bRevealWhenPresentationReady)
		Unit->SetUnitPresentationVisible_Client(true);
}

void USpUnitClientGatewayComponent::StopPresentation_Client()
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	if (bPresentationReady)
	{
		if (ASpGameState* GameState = Unit->GetWorld()->GetGameState<ASpGameState>())
			GameState->RemoveUnitPresentationReady_Client(PresentationUnitUid);
	}

	Unit->SetUnitPresentationVisible_Client(false);

	if (PresentationUnitUid != 0)
	{
		TInlineComponentArray<UActorComponent*> Components(Unit);
		for (UActorComponent* Component : Components)
		{
			if (ISpUnitClientListener* Listener = Cast<ISpUnitClientListener>(Component))
				Listener->StopClientPresentation();
		}
	}

	PendingPresentationListeners.Reset();
	bPresentationReady = false;
	PresentationUnitUid = 0;
}

// get

ASpUnit* USpUnitClientGatewayComponent::GetOwnerUnitChecked() const
{
	ASpUnit* Unit = GetOwner<ASpUnit>();
	
	check(Unit);
	check(Unit->GetNetMode() != NM_DedicatedServer);

	return Unit;
}
