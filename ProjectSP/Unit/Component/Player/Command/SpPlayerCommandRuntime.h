#pragma once

#include "CoreMinimal.h"
#include "SpPlayerCommandTypes.h"
#include "ImplementedCommand/SpPlayerPrimaryAttackCommand.h"

class ASpUnit;
class USpAbilitySystemComponent;
class FSpPlayerCommandService_Movement;
class FSpPlayerCommandResolver;

// ==================================================

// 모든 서버 명령이 공통으로 사용하는 최소 실행 자원.
struct FSpPlayerCommandContext_Server
{
	ASpUnit& Unit;
	FSpPlayerCommandService_Movement& MovementService;
};

// ==================================================

class FSpPlayerCommandRuntime_Server
{
	FSpPlayerCommandService_Movement* MovementService = nullptr;
	FSpPlayerPrimaryAttackRules PrimaryAttackRules;
	TUniquePtr<ISpPlayerCommand> ActiveCommand;

public:
	FSpPlayerCommandRuntime_Server() = default;
	
	explicit FSpPlayerCommandRuntime_Server(FSpPlayerCommandService_Movement& InMovementService)
		: MovementService(&InMovementService)
	{
	}
	
	void Initialize(FSpPlayerCommandService_Movement& InMovementService) { MovementService = &InMovementService; }
	void Configure(FSpPlayerPrimaryAttackRules InPrimaryAttackRules) { PrimaryAttackRules = MoveTemp(InPrimaryAttackRules); }

	void HandleInput_Server(ASpUnit& Unit, const FSpPlayerCommandInput& CommandInput, const FSpPlayerCommandResolver& Resolver);
	void Tick_Server(ASpUnit& Unit, float DeltaTime);
	bool ValidatePredictedAbility_Server(ASpUnit& Unit, uint16 CommandId, FGameplayTag AbilityTag);
	void Clear_Server(ASpUnit& Unit);

	bool IsActive_Server() const { return ActiveCommand.IsValid(); }
	AActor* GetTargetActor() const { return ActiveCommand ? ActiveCommand->GetTargetActor() : nullptr; }

private:
	TUniquePtr<ISpPlayerCommand> CreateServerCommand(const uint16 CommandId, const FSpPlayerCommandResolution& Resolution, const FSpPlayerPrimaryAttackRules& Rules);
	void StartCommand_Server(ASpUnit& Unit, TUniquePtr<ISpPlayerCommand> InCommand);
	void StopCommand_Server(ASpUnit& Unit);
};

// ==================================================

class FSpPlayerCommandRuntime_Client
{
	FSpPlayerCommandService_Movement* MovementService = nullptr;
	FSpPlayerPrimaryAttackRules PrimaryAttackRules;
	uint16 ActiveCommandId = 0;
	TUniquePtr<ISpPlayerCommandPrediction> ActivePrediction;

public:
	FSpPlayerCommandRuntime_Client() = default;

	explicit FSpPlayerCommandRuntime_Client(FSpPlayerCommandService_Movement& InMovementService)
		: MovementService(&InMovementService)
	{
	}
	
	void Initialize(FSpPlayerCommandService_Movement& InMovementService) { MovementService = &InMovementService; }
	void Configure(FSpPlayerPrimaryAttackRules InPrimaryAttackRules) { PrimaryAttackRules = MoveTemp(InPrimaryAttackRules); }

	void HandleInput_Client(ASpUnit& Unit, USpAbilitySystemComponent& ASC, const FSpPlayerCommandInput& CommandInput, const FSpPlayerCommandResolver& Resolver);
	void Tick_Client(ASpUnit& Unit, float DeltaTime);
	void Clear_Client();
	bool ShouldTick_Client() const;

private:
	TUniquePtr<ISpPlayerCommandPrediction> CreatePrediction_Client(const uint16 CommandId, ASpUnit& SourceUnit, USpAbilitySystemComponent& ASC, const FSpPlayerCommandResolution& Resolution, const FSpPlayerPrimaryAttackRules& Rules);
};
