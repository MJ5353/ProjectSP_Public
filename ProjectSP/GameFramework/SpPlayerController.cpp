#include "SpPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "ProjectSP/Ability/Extensions/SpAbilityExtensionComponent.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/Unit/LocalPlayer/SpInputMappingContext.h"
#include "ProjectSP/GameFramework/SpGameMode.h"
#include "ProjectSP/GameFramework/SpGameState.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"

// ==================================================

ASpPlayerController::ASpPlayerController()
{
	AbilityExtensionComponent = CreateDefaultSubobject<USpAbilityExtensionComponent>(TEXT("SpAbilityExtensionComponent"));
}

void ASpPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	SyncAbilityExtensions_Server(true);
}

void ASpPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ApplyUnitInputMappings_Client(nullptr);
	Super::EndPlay(EndPlayReason);
}

// server

void ASpPlayerController::SyncAbilityExtensions_Server(const bool bPawnChanged)
{
	if (!HasAuthority() || !AbilityExtensionComponent)
		return;

	ASpPlayerUnit* Unit = GetPawn<ASpPlayerUnit>();
	if (!IsValid(Unit) || Unit->GetController() != this)
	{
		if (bPawnChanged)
			AbilityExtensionComponent->SetUp_Server(nullptr, true);

		return;
	}

	const USpUnitDefinition* UnitDefinition = Unit->GetUnitDefinition();
	AbilityExtensionComponent->SetUp_Server(UnitDefinition, bPawnChanged);
}

void ASpPlayerController::ServerSetReady_Implementation(const bool bReadyToStart)
{
	if (UWorld* World = GetWorld())
	{
		if (ASpGameMode* GameMode = World->GetAuthGameMode<ASpGameMode>())
			GameMode->HandlePlayerReadyChanged_Server(this, bReadyToStart);
	}
}

void ASpPlayerController::ServerRequestStart_Implementation()
{
	// 현재 방장만 요청할 수 있으며, 서버가 전원 Ready 여부를 다시 검증한다.

	if (UWorld* World = GetWorld())
	{
		if (ASpGameMode* GameMode = World->GetAuthGameMode<ASpGameMode>())
			GameMode->HandleRoomStartRequest_Server(this);
	}
}

void ASpPlayerController::ServerReportInitialPresentationReady_Implementation(const uint32 InitialPresentationId)
{
	// 초기 유닛 표현 준비가 끝난 로컬 클라이언트가 서버에 보내는 완료 보고

	if (UWorld* World = GetWorld())
	{
		if (ASpGameMode* GameMode = World->GetAuthGameMode<ASpGameMode>())
			GameMode->HandleInitialPresentationReady_Server(this, InitialPresentationId);
	}
}

void ASpPlayerController::ServerPurchaseAbilityExtension_Implementation(const FGameplayTag ExtensionTag)
{
	const ESpAbilityExtensionPurchaseResult Result = AbilityExtensionComponent ? AbilityExtensionComponent->TryPurchaseExtension_Server(ExtensionTag) : ESpAbilityExtensionPurchaseResult::NoActiveUnit;
	ClientAbilityExtensionPurchaseResult(ExtensionTag, Result);
}

void ASpPlayerController::ClientAbilityExtensionPurchaseResult_Implementation(const FGameplayTag ExtensionTag, const ESpAbilityExtensionPurchaseResult Result)
{
	OnAbilityExtensionPurchaseResolved.Broadcast(ExtensionTag, Result);
}

void ASpPlayerController::ServerSelectPlayableUnit_Implementation(USpUnitDefinition* UnitDefinition)
{
	if (ASpGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<ASpGameMode>() : nullptr)
		GameMode->HandlePlayableUnitSelection_Server(this, UnitDefinition);
}

// get

bool ASpPlayerController::IsGameplayInputEnabled_Client() const
{
	const UWorld* World = GetWorld();
	const ASpGameState* GameState = World ? World->GetGameState<ASpGameState>() : nullptr;

	// GameState가 Playing일 때만 로컬 게임플레이 입력을 허용
	return GameState && GameState->IsGameplayActive();
}

bool ASpPlayerController::ApplyUnitInputMappings_Client(const USpUnitDefinition* UnitDefinition)
{
	if (!IsLocalController())
		return false;

	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!InputSubsystem)
		return false;

	for (const TObjectPtr<const UInputMappingContext>& Context : ActiveUnitInputMappings)
	{
		if (Context)
			InputSubsystem->RemoveMappingContext(Context.Get());
	}
	ActiveUnitInputMappings.Reset();

	if (!IsValid(UnitDefinition))
		return true;

	FModifyContextOptions Options = {};
	Options.bIgnoreAllPressedKeysUntilRelease = false;

	// unit input
	for (const FSpInputMappingContext& Mapping : UnitDefinition->InputMappingContexts)
	{
		const UInputMappingContext* Context = Mapping.Context.Get();
		auto Predicate = [Context](const TObjectPtr<const UInputMappingContext>& ActiveContext)
		{
			return ActiveContext.Get() == Context;
		};
		
		if (!Mapping.bShouldActivateAutomatically || !Context || ActiveUnitInputMappings.ContainsByPredicate(Predicate))
			continue;

		InputSubsystem->AddMappingContext(Context, 0, Options);
		ActiveUnitInputMappings.Add(Context);
	}

	return true;
}
