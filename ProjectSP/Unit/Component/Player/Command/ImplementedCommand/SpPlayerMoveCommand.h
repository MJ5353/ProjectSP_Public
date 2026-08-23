#pragma once

#include "CoreMinimal.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandTypes.h"

// ==================================================

struct FSpPlayerCommandContext_Server;

class FSpPlayerMoveCommand final : public ISpPlayerCommand
{
	uint16 CommandId = 0;
	uint16 LastCursorSequence = 0;
	FVector MoveDestination = FVector::ZeroVector;

public:
	FSpPlayerMoveCommand(uint16 InCommandId, const FVector& InMoveDestination);

	virtual uint16 GetId() const override { return CommandId; }
	
	virtual bool Begin(FSpPlayerCommandContext_Server& Context) override;
	virtual bool Update(FSpPlayerCommandContext_Server& Context, const FSpPlayerCommandResolvedInput& Input) override;
	virtual bool Tick(FSpPlayerCommandContext_Server& Context, float DeltaTime) override;
	virtual ESpPlayerCommandAbilityValidation ValidateAbility(FSpPlayerCommandContext_Server& Context, FGameplayTag AbilityTag) override;
};
