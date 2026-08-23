#include "SpPlayerCommandComponent.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

USpPlayerCommandComponent::USpPlayerCommandComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ServerRuntime(MovementService)
	, ClientRuntime(MovementService)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void USpPlayerCommandComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigureRuntime();
}

void USpPlayerCommandComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
	{
		SetComponentTickEnabled(false);
		return;
	}

	if (Unit->HasAuthority())
	{
		if (IsGameplayActive_Server())
			ServerRuntime.Tick_Server(*Unit, DeltaTime);
		else
			ServerRuntime.Clear_Server(*Unit);
	}

	if (Unit->IsLocallyControlled())
		ClientRuntime.Tick_Client(*Unit, DeltaTime);

	UpdateTickEnabled();
}

// unit manage

void USpPlayerCommandComponent::OnClearUnit()
{
	ClearCommand();
}

void USpPlayerCommandComponent::OnUnitActive(const bool bActive)
{
	ClearCommand();
}

void USpPlayerCommandComponent::OnUnitStateChanged(const FGameplayTag StateTag, const bool bAdded)
{
	if (bAdded && StateTag.MatchesTagExact(FSpGameplayTags::Get().StateTag_Dead))
		ClearCommand();
}

// command boundary

void USpPlayerCommandComponent::SubmitCommand_Client(const FSpPlayerCommandInput& CommandInput)
{
	if (CommandInput.CommandId == 0)
		return;

	ConfigureRuntime();

	ASpUnit* Unit = GetPawn<ASpUnit>();
	USpAbilitySystemComponent* AbilitySystemComponent = Unit ? Unit->GetSpAbilitySystemComponent() : nullptr;
	if (!Unit || !AbilitySystemComponent)
		return;

	AbilitySystemComponent->SubmitPlayerCommand_Client(CommandInput);
	ClientRuntime.HandleInput_Client(*Unit, *AbilitySystemComponent, CommandInput, Resolver);
	ClientRuntime.Tick_Client(*Unit, 0.0f);
	UpdateTickEnabled();
}

void USpPlayerCommandComponent::HandleCommand_Server(const FSpPlayerCommandInput& CommandInput)
{
	if (CommandInput.CommandId == 0 || !IsGameplayActive_Server())
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit || !Unit->HasAuthority())
		return;

	ConfigureRuntime();
	ServerRuntime.HandleInput_Server(*Unit, CommandInput, Resolver);
	UpdateTickEnabled();
}

bool USpPlayerCommandComponent::ValidatePredictedAbilityForCommand_Server(const uint16 CommandId, const FGameplayTag AbilityTag)
{
	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit || !Unit->HasAuthority())
		return false;

	ConfigureRuntime();
	return ServerRuntime.ValidatePredictedAbility_Server(*Unit, CommandId, AbilityTag);
}

void USpPlayerCommandComponent::ApplyMovementState_Client(const uint16 CommandId, const bool bHasWaypoint, const FVector& Waypoint)
{
	ConfigureRuntime();
	MovementService.ApplyMoveState_Client(CommandId, bHasWaypoint, Waypoint);
	UpdateTickEnabled();
}

// lifecycle

void USpPlayerCommandComponent::ConfigureRuntime()
{
	if (bRuntimeConfigured)
		return;

	FSpPlayerCommandMovementSettings MovementSettings;
	MovementSettings.RepathDistance = MoveRepathDistance;
	MovementSettings.RepathInterval = MoveRepathInterval;
	MovementSettings.EndDistance = MoveEndDistance;
	MovementService.Configure(MovementSettings);

	FSpPlayerCommandResolverSettings ResolverSettings;
	ResolverSettings.CursorTraceDistance = CursorTraceDistance;
	ResolverSettings.NavigationProjectionExtent = NavigationProjectionExtent;
	Resolver.Configure(ResolverSettings);

	const FSpPlayerPrimaryAttackRules PrimaryAttackRules(BasicAttackRange, AttackMoveRepathDistance);
	ServerRuntime.Configure(PrimaryAttackRules);
	ClientRuntime.Configure(PrimaryAttackRules);
	bRuntimeConfigured = true;
}

void USpPlayerCommandComponent::ClearCommand()
{
	ClientRuntime.Clear_Client();

	if (ASpUnit* Unit = GetPawn<ASpUnit>(); Unit && Unit->HasAuthority())
		ServerRuntime.Clear_Server(*Unit);

	UpdateTickEnabled();
}

void USpPlayerCommandComponent::UpdateTickEnabled()
{
	const ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return;

	const bool bNeedsServerTick = Unit->HasAuthority() && ServerRuntime.IsActive_Server() && IsGameplayActive_Server();
	const bool bNeedsClientTick = Unit->IsLocallyControlled() && ClientRuntime.ShouldTick_Client();
	
	SetComponentTickEnabled(bNeedsServerTick || bNeedsClientTick);
}

bool USpPlayerCommandComponent::IsGameplayActive_Server() const
{
	const ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit || !Unit->HasAuthority())
		return false;

	const UWorld* World = GetWorld();
	const ASpGameState* GameState = World ? World->GetGameState<ASpGameState>() : nullptr;
	
	return GameState && GameState->IsGameplayActive();
}
