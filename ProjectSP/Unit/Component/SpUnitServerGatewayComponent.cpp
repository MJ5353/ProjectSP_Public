#include "SpUnitServerGatewayComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Interface/SpUnitServerListener.h"

// ==================================================

void USpUnitServerGatewayComponent::ApplyUnitData_Server(const FSpUnitData& UnitData) const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	check(UnitData.IsValid());

	Unit->ApplyUnitData_Server(UnitData);
}

void USpUnitServerGatewayComponent::PrepareUnit_Server() const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	check(Unit->GetUnitData().IsValid());
	
	NotifyPrepared_Server();
}

void USpUnitServerGatewayComponent::ActivateUnit_Server()
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	check(Unit->GetUnitData().IsValid());

	Unit->SetUnitActive_Server(true, true);
	NotifyActivated_Server(Unit->GetUnitData());
}

void USpUnitServerGatewayComponent::ReturnUnit_Server()
{
	ASpUnit* Unit = GetOwnerUnitChecked();

	Unit->SetUnitActive_Server(false, true);
	Unit->ResetUnitData_Server();
	NotifyReturned_Server();
}

// notify

void USpUnitServerGatewayComponent::NotifyPrepared_Server() const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	const FSpUnitData& UnitData = Unit->GetUnitData();
	
	TInlineComponentArray<UActorComponent*> Components(Unit);
	for (UActorComponent* Component : Components)
	{
		if (ISpUnitServerListener* Listener = Cast<ISpUnitServerListener>(Component))
			Listener->OnServerUnitPrepared(UnitData);
	}
}

void USpUnitServerGatewayComponent::NotifyActivated_Server(const FSpUnitData& UnitData) const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	
	TInlineComponentArray<UActorComponent*> Components(Unit);
	for (UActorComponent* Component : Components)
	{
		if (ISpUnitServerListener* Listener = Cast<ISpUnitServerListener>(Component))
			Listener->OnServerUnitActivated(UnitData);
	}
}

void USpUnitServerGatewayComponent::NotifyReturned_Server() const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	
	TInlineComponentArray<UActorComponent*> Components(Unit);
	for (UActorComponent* Component : Components)
	{
		if (ISpUnitServerListener* Listener = Cast<ISpUnitServerListener>(Component))
			Listener->OnServerUnitReturned();
	}
}

// get

ASpUnit* USpUnitServerGatewayComponent::GetOwnerUnitChecked() const
{
	ASpUnit* Unit = GetOwner<ASpUnit>();

	check(Unit);
	check(Unit->HasAuthority());
	
	return Unit;
}
