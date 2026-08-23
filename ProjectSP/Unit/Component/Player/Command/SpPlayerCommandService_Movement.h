#pragma once

#include "CoreMinimal.h"

class ASpUnit;

// ==================================================

struct FSpPlayerCommandMovementSettings
{
	float RepathDistance = 100.0f;
	float RepathInterval = 0.1f;
	float EndDistance = 50.0f;
};

// ==================================================

// PlayerCommand이 이동을 요구할 때 사용하는 서버 경로 실행과 Client 이동 재생 서비스.
// 서버 경로와 Client MoveState는 이 서비스가 소유한다.

class FSpPlayerCommandService_Movement
{
	FSpPlayerCommandMovementSettings Settings;
	TArray<FVector> ServerPathPoints;
	int32 ServerPathIndex = 0;
	float NextServerRepathTime = 0.0f;

	uint16 ClientCommandId = 0;
	bool bClientHasWaypoint = false;
	FVector ClientWaypoint = FVector::ZeroVector;

public:
	void Configure(FSpPlayerCommandMovementSettings InSettings) { Settings = InSettings; }

	// server
	bool Tick_Server(ASpUnit& Unit, uint16 CommandId, float DeltaTime);
	bool UpdatePathDestination_Server(ASpUnit& Unit, uint16 CommandId, const FVector& Destination);
	bool RebuildPath_Server(ASpUnit& Unit, uint16 CommandId, const FVector& Destination);
	void StopFollowing_Server(ASpUnit& Unit, uint16 CommandId);
	void StopCommand_Server(ASpUnit& Unit, uint16 CommandId);

	// client
	void BeginCommand_Client(uint16 CommandId);
	void ApplyMoveState_Client(uint16 CommandId, bool bHasWaypoint, const FVector& Waypoint);
	void EndCommand_Client(uint16 CommandId);
	void ClearClientState_Client();
	bool Tick_Client(ASpUnit& Unit, float DeltaTime) const;
	bool HasWaypoint_Client() const { return bClientHasWaypoint; }

	// check
	bool ShouldRepath_Server(const ASpUnit& Unit, const FVector& Destination) const;
	bool HasPath_Server() const;
	const FVector& GetPathEnd_Server() const;

private:
	void SendMoveStateToClient_Server(ASpUnit& Unit, uint16 CommandId, bool bHasWaypoint, const FVector& Waypoint) const;
	void ClearPath_Server();
};
