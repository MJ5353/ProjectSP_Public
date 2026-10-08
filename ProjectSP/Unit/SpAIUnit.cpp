#include "SpAIUnit.h"

#include "Component/SpUnitServerGatewayComponent.h"
#include "Component/Common/SpUnitAggroComponent.h"
#include "ProjectSP/Subsystem/ActorPool/ActorPoolSubsystem.h"

// ==================================================

ASpAIUnit::ASpAIUnit(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AggroComponent = CreateDefaultSubobject<USpUnitAggroComponent>(TEXT("SpUnitAggroComponent"));
}

void ASpAIUnit::Push()
{
	UActorPoolSubsystem* ActorPoolSubsystem = UActorPoolSubsystem::Get(GetWorld());
	if (!ActorPoolSubsystem)
		return;

	ActorPoolSubsystem->ReturnActor(this);
}

void ASpAIUnit::OnDestroy()
{
	ClearUnit();
}

void ASpAIUnit::OnReturn()
{
	if (UWorld* World = GetWorld())
		World->GetTimerManager().ClearTimer(DeadProcessTimerHandle);

	bDeadPresentationActive = false;
	
	if (HasAuthority() && ServerGateway)
		ServerGateway->ReturnUnit_Server();
}

void ASpAIUnit::HandleDeadProcessFinished_Server()
{
	check(HasAuthority());

	Push();
}
