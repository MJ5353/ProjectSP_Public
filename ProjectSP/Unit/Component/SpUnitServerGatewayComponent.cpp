#include "SpUnitServerGatewayComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Interface/SpUnitServerListener.h"

// ==================================================

void USpUnitServerGatewayComponent::PrepareUnit_ServerOnly(const FSpUnitData& UnitData)
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	check(UnitData.IsValid());

	Unit->ApplyUnitData_ServerOnly(UnitData);
	NotifyPrepared_ServerOnly(UnitData);
}

void USpUnitServerGatewayComponent::ActivateUnit_ServerOnly()
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	check(Unit->GetUnitData().IsValid());

	Unit->SetUnitActive_ServerOnly(true);
	NotifyActivated_ServerOnly(Unit->GetUnitData());
}

void USpUnitServerGatewayComponent::ReturnUnit_ServerOnly()
{
	ASpUnit* Unit = GetOwnerUnitChecked();

	Unit->SetUnitActive_ServerOnly(false);
	Unit->ResetUnitData_ServerOnly();
	NotifyReturned_ServerOnly();
}

// notify

void USpUnitServerGatewayComponent::NotifyPrepared_ServerOnly(const FSpUnitData& UnitData) const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	
	TInlineComponentArray<UActorComponent*> Components(Unit);
	for (UActorComponent* Component : Components)
	{
		if (ISpUnitServerListener* Listener = Cast<ISpUnitServerListener>(Component))
			Listener->OnServerUnitPrepared(UnitData);
	}
}

void USpUnitServerGatewayComponent::NotifyActivated_ServerOnly(const FSpUnitData& UnitData) const
{
	ASpUnit* Unit = GetOwnerUnitChecked();
	
	TInlineComponentArray<UActorComponent*> Components(Unit);
	for (UActorComponent* Component : Components)
	{
		if (ISpUnitServerListener* Listener = Cast<ISpUnitServerListener>(Component))
			Listener->OnServerUnitActivated(UnitData);
	}
}

void USpUnitServerGatewayComponent::NotifyReturned_ServerOnly() const
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
