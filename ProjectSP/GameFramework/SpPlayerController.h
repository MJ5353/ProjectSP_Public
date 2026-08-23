#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ProjectSP/Unit/Component/Player/Command/SpPlayerCommandTypes.h"
#include "SpPlayerController.generated.h"

class USpAbilitySystemComponent;
class ASpPlayerState;

// ==================================================

UCLASS()
class PROJECTSP_API ASpPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	
	// server
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetReady(bool bReadyToStart);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerRequestStart();

	UFUNCTION(Server, Reliable)
	void ServerReportInitialPresentationReady(uint32 InitialPresentationId);

	UFUNCTION(Client, Reliable)
	void ClientSetCommandMoveState(uint16 CommandId, bool bHasWaypoint, FVector_NetQuantize10 Waypoint);

	// get
	bool IsGameplayInputEnabled_Client() const;
	USpAbilitySystemComponent* GetSpAbilitySystemComponent() const;
};
