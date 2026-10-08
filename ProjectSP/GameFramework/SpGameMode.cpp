#include "SpGameMode.h"
#include "SpPlayerController.h"
#include "SpPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/GameFramework/GameState/SpGamePhaseState.h"
#include "ProjectSP/GameFramework/GameState/SpGamePhaseLobbyState.h"
#include "ProjectSP/Subsystem/SpawnSubsystem.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/Define/SpTeam.h"

// ==================================================

namespace
{
float GetPlayerCapsuleRadius(const ASpPlayerController* PlayerController, const float FallbackRadius)
{
	const ASpPlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<ASpPlayerState>() : nullptr;
	const USpUnitDefinition* UnitDefinition = PlayerState ? PlayerState->GetUnitDefinition() : nullptr;
	const ASpPlayerUnit* UnitCDO = UnitDefinition && UnitDefinition->UnitClass && UnitDefinition->UnitClass->IsChildOf(ASpPlayerUnit::StaticClass()) ? UnitDefinition->UnitClass->GetDefaultObject<ASpPlayerUnit>() : nullptr;
	const UCapsuleComponent* Capsule = UnitCDO ? UnitCDO->GetCapsuleComponent() : nullptr;
	return Capsule ? Capsule->GetScaledCapsuleRadius() : FallbackRadius;
}

bool ResolveOrderedPlayerSpawnTransform(UWorld* World, const USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, const int32 SlotIndex, const TArray<TWeakObjectPtr<ASpPlayerController>>& SlotOwners, FTransform& OutTransform)
{
	constexpr float SideGap = 20.f;
	constexpr float GroundClearance = 2.f;
	constexpr int32 RightSideAttemptCount = 3;
	if (!World || SlotIndex < 0 || !UnitDefinition || !UnitDefinition->UnitClass || !UnitDefinition->UnitClass->IsChildOf(ASpPlayerUnit::StaticClass()))
		return false;

	const ASpPlayerUnit* UnitCDO = UnitDefinition->UnitClass->GetDefaultObject<ASpPlayerUnit>();
	const UCapsuleComponent* Capsule = UnitCDO ? UnitCDO->GetCapsuleComponent() : nullptr;
	if (!Capsule)
		return false;

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	const FVector Origin = SpawnTransform.GetLocation();
	FVector RightDirection = SpawnTransform.GetUnitAxis(EAxis::Y);
	RightDirection.Z = 0.f;
	if (!RightDirection.Normalize())
		RightDirection = FVector::RightVector;

	float RightOffset = 0.f;
	float PreviousRadius = GetPlayerCapsuleRadius(SlotOwners.IsValidIndex(0) ? SlotOwners[0].Get() : nullptr, CapsuleRadius);
	for (int32 Index = 1; Index <= SlotIndex; ++Index)
	{
		const float NextRadius = Index == SlotIndex ? CapsuleRadius : GetPlayerCapsuleRadius(SlotOwners.IsValidIndex(Index) ? SlotOwners[Index].Get() : nullptr, CapsuleRadius);
		RightOffset += PreviousRadius + NextRadius + SideGap;
		PreviousRadius = NextRadius;
	}

	const int32 AttemptCount = SlotIndex == 0 ? 1 : RightSideAttemptCount;
	const float RetryDistance = 2.f * CapsuleRadius + SideGap;
	for (int32 Attempt = 0; Attempt < AttemptCount; ++Attempt)
	{
		FTransform CandidateTransform = SpawnTransform;
		CandidateTransform.SetLocation(Origin + RightDirection * (RightOffset + Attempt * RetryDistance) + FVector::UpVector * GroundClearance);
		const FVector CandidateLocation = CandidateTransform.GetLocation();
		if (World->OverlapBlockingTestByProfile(CandidateLocation, CandidateTransform.GetRotation(), Capsule->GetCollisionProfileName(), CapsuleShape))
			continue;

		bool bOverlapsUnit = false;
		for (TActorIterator<ASpUnit> It(World); It; ++It)
		{
			const ASpUnit* OtherUnit = *It;
			if (!IsValid(OtherUnit) || !OtherUnit->HasValidUnitData())
				continue;

			const UCapsuleComponent* OtherCapsule = OtherUnit->GetCapsuleComponent();
			if (!OtherCapsule)
				continue;

			const FVector OtherLocation = OtherUnit->GetActorLocation();
			const float CombinedRadius = CapsuleRadius + OtherCapsule->GetScaledCapsuleRadius();
			const float CombinedHalfHeight = CapsuleHalfHeight + OtherCapsule->GetScaledCapsuleHalfHeight();
			if (FVector::DistSquared2D(CandidateLocation, OtherLocation) < FMath::Square(CombinedRadius)
				&& FMath::Abs(CandidateLocation.Z - OtherLocation.Z) < CombinedHalfHeight)
			{
				bOverlapsUnit = true;
				break;
			}
		}

		if (bOverlapsUnit)
			continue;

		OutTransform = CandidateTransform;
		return true;
	}

	return false;
}
}

// ==================================================

ASpGameMode::ASpGameMode() : PhaseMachine(Phase)
{
	GameStateClass = ASpGameState::StaticClass();
	PrimaryActorTick.bCanEverTick = true;
}

void ASpGameMode::InitGameState()
{
	Super::InitGameState();

	if (ensure(GetGameState<ASpGameState>()))
		PhaseMachine.Begin<FSpGamePhaseLobbyState>(*this);
}

void ASpGameMode::StartPlay()
{
	Super::StartPlay();

	if (FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>())
		State->StartPlay();
}

void ASpGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PhaseMachine.End();
	Super::EndPlay(EndPlayReason);
}

void ASpGameMode::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>())
	{
		// 상태의 처리가 반환된 뒤 교체한다. 콜백 실행 중 상태 객체를 파괴하지 않는다.
		if (TUniquePtr<ISpState<ESpGamePhase>> NextState = State->Update())
			PhaseMachine.TryChangeState(MoveTemp(NextState));
	}
}

void ASpGameMode::Logout(AController* Exiting)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(Exiting);
	
	if (State && PlayerController)
		State->BeforePlayerLogout(*PlayerController);
	if (PlayerController)
	{
		for (TWeakObjectPtr<ASpPlayerController>& SlotOwner : PlayerSpawnSlotOwners)
		{
			if (SlotOwner.Get() == PlayerController)
			{
				SlotOwner.Reset();
				break;
			}
		}
	}

	Super::Logout(Exiting);

	State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->AfterPlayerLogout();
}

// override gameMode

bool ASpGameMode::ReadyToStartMatch_Implementation()
{
	return PhaseMachine.IsActive() && Phase == ESpGamePhase::Playing;
}

void ASpGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(NewPlayer);

	if (!State || !PlayerController)
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);
		return;
	}

	ClaimPlayerSpawnSlot(*PlayerController);
	const bool bRunDefaultStart = State->BeforePlayerJoin(*PlayerController);

	if (bRunDefaultStart)
		Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	State->HandlePlayerJoined(*PlayerController);
}

UClass* ASpGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	if (const ASpPlayerState* PlayerState = InController->GetPlayerState<ASpPlayerState>())
	{
		if (USpUnitDefinition* UnitDefinition = PlayerState->GetUnitDefinition())
			return UnitDefinition->UnitClass;
	}

	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

APawn* ASpGameMode::SpawnDefaultPawnAtTransform_Implementation(AController* NewPlayer, const FTransform& SpawnTransform)
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = GetInstigator();
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;

	ASpPlayerController* PlayerController = Cast<ASpPlayerController>(NewPlayer);
	const ASpPlayerState* PlayerState = NewPlayer->GetPlayerState<ASpPlayerState>();

	if (!PlayerController || !PlayerState)
		return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);

	if (USpUnitDefinition* UnitDefinition = PlayerState->GetUnitDefinition())
	{
		if (USpawnSubsystem* SpawnSubsystem = GetWorld()->GetSubsystem<USpawnSubsystem>())
		{
			const int32 SlotIndex = ClaimPlayerSpawnSlot(*PlayerController);
			FTransform OrderedSpawnTransform;
			if (!ResolveOrderedPlayerSpawnTransform(GetWorld(), UnitDefinition, SpawnTransform, SlotIndex, PlayerSpawnSlotOwners, OrderedSpawnTransform))
				return nullptr;

			bool bActivateImmediately = Phase == ESpGamePhase::Playing;
			FGenericTeamId TeamID = FGenericTeamId(SpTeam::PlayerId);

			return SpawnSubsystem->SpawnPlayerUnit(UnitDefinition, OrderedSpawnTransform, SpawnInfo, TeamID, bActivateImmediately);
		}
		return nullptr;
	}

	return Super::SpawnDefaultPawnAtTransform_Implementation(NewPlayer, SpawnTransform);
}

// handle

int32 ASpGameMode::ClaimPlayerSpawnSlot(ASpPlayerController& PlayerController)
{
	for (int32 Index = 0; Index < PlayerSpawnSlotOwners.Num(); ++Index)
	{
		if (PlayerSpawnSlotOwners[Index].Get() == &PlayerController)
			return Index;
	}

	for (int32 Index = 0; Index < PlayerSpawnSlotOwners.Num(); ++Index)
	{
		if (!PlayerSpawnSlotOwners[Index].IsValid())
		{
			PlayerSpawnSlotOwners[Index] = &PlayerController;
			return Index;
		}
	}

	return PlayerSpawnSlotOwners.Add(TWeakObjectPtr<ASpPlayerController>(&PlayerController));
}

void ASpGameMode::HandlePlayerReadyChanged_Server(ASpPlayerController* PlayerController, const bool bReadyToStart)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->HandlePlayerReadyChanged(*PlayerController, bReadyToStart);
}

void ASpGameMode::HandleRoomStartRequest_Server(ASpPlayerController* PlayerController)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->HandleStartRequested(*PlayerController);
}

void ASpGameMode::HandleInitialPresentationReady_Server(ASpPlayerController* PlayerController, const uint32 InInitialPresentationId)
{
	FSpGamePhaseState* State = PhaseMachine.GetActiveStateAs<FSpGamePhaseState>();
	if (State && PlayerController)
		State->HandlePresentationReady(*PlayerController, InInitialPresentationId);
}

bool ASpGameMode::HandlePlayableUnitSelection_Server(ASpPlayerController* PlayerController, USpUnitDefinition* UnitDefinition)
{
	if (!HasAuthority() || !IsValid(PlayerController) || PlayerController->GetWorld() != GetWorld())
		return false;

	ASpPlayerState* PlayerState = PlayerController->GetPlayerState<ASpPlayerState>();
	const ASpGameState* SpGameState = GetGameState<ASpGameState>();

	if (!PlayerState || !SpGameState || !SpGameState->IsPlayableUnitDefinition(UnitDefinition))
		return false;

	if (Phase == ESpGamePhase::Lobby)
	{
		PlayerState->SetSelectedUnitDefinition_Server(UnitDefinition);
		return true;
	}

	if (Phase != ESpGamePhase::Playing)
		return false;
	
	return ReplacePlayableUnit_Server(*PlayerController, *PlayerState, UnitDefinition);
}

bool ASpGameMode::ReplacePlayableUnit_Server(ASpPlayerController& PlayerController, ASpPlayerState& PlayerState, USpUnitDefinition* UnitDefinition)
{
	ASpPlayerUnit* PreviousUnit = PlayerController.GetPawn<ASpPlayerUnit>();
	if (!IsValid(PreviousUnit) || PreviousUnit->GetController() != &PlayerController || !PreviousUnit->HasValidUnitData())
		return false;

	if (PreviousUnit->GetUnitDefinition() == UnitDefinition)
	{
		PlayerState.SetSelectedUnitDefinition_Server(UnitDefinition);
		return true;
	}

	USpawnSubsystem* SpawnSubsystem = GetWorld()->GetSubsystem<USpawnSubsystem>();
	if (!SpawnSubsystem)
		return false;

	ASpPlayerUnit* NewUnit = SpawnReplacementUnit_Server(*SpawnSubsystem, *PreviousUnit, UnitDefinition);
	if (!NewUnit)
		return false;

	PlayerController.Possess(NewUnit);
	if (PlayerController.GetPawn() != NewUnit)
	{
		NewUnit->Destroy();
		if (IsValid(PreviousUnit) && PlayerController.GetPawn() != PreviousUnit)
			PlayerController.Possess(PreviousUnit);

		return false;
	}

	SpawnSubsystem->ActivateUnit(NewUnit, false);
	PlayerState.SetSelectedUnitDefinition_Server(UnitDefinition);
	PreviousUnit->Destroy();

	return true;
}

ASpPlayerUnit* ASpGameMode::SpawnReplacementUnit_Server(USpawnSubsystem& SpawnSubsystem, const ASpPlayerUnit& PreviousUnit, USpUnitDefinition* UnitDefinition)
{
	FTransform SpawnTransform = PreviousUnit.GetActorTransform();
	SpawnTransform.SetLocation(PreviousUnit.GetSpawnLocation());

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.ObjectFlags |= RF_Transient;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	return SpawnSubsystem.SpawnPlayerUnit(UnitDefinition, SpawnTransform, SpawnInfo, FGenericTeamId(SpTeam::PlayerId), false);
}
