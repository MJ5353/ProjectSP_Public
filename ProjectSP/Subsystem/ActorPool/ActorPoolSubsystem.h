#pragma once

#include "CoreMinimal.h"
#include "SpActorPoolDefine.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

struct FActorPoolBucket;
struct FSpActorPoolKey;
struct FSpUnitSpawnParams;

// ==================================================

UCLASS()
class PROJECTSP_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TMap<FSpActorPoolKey, FActorPoolBucket> InactiveActors;
	
	UPROPERTY(Transient)
	TMap<TObjectPtr<AActor>, FSpActorPoolInfo> SpawnedActors;
	
	virtual void Deinitialize() override;
	
public:
	static UActorPoolSubsystem* Get(const UWorld* World);
	
	// spawn
	template <class TActor>
	TActor* SpawnActor_Instant(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParams);
	AActor* SpawnActor_Instant(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParams);
	
	template <class TActor>
	TActor* BeginSpawnActor_Deferred(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParams);
	AActor* BeginSpawnActor_Deferred(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParams);
	
	// after spawn
	void FinishSpawnActor_Deferred(AActor* Actor, const FTransform& Transform);
	void ActivateActor(AActor* Actor); // spawn 후 별도로 activate를 요청한다.
	void ReturnActor(AActor* Actor);

private:
	void SetActiveActor(AActor* Actor, bool bActive);
	
	UFUNCTION()
	void HandlePooledActorDestroyed(AActor* Actor);
};

// ------------------------------------------------

template <class TActor>
TActor* UActorPoolSubsystem::SpawnActor_Instant(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParams)
{
	AActor* Actor = SpawnActor_Instant(PoolKey, SpawnTransform, SpawnParams);
	if (!Actor)
		return nullptr;
	
	if (TActor* Result = Cast<TActor>(Actor))
		return Result;
	
	return nullptr;
}

template <class TActor>
TActor* UActorPoolSubsystem::BeginSpawnActor_Deferred(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParams)
{
	AActor* Actor = BeginSpawnActor_Deferred(PoolKey, SpawnTransform, SpawnParams);
	if (!Actor)
		return nullptr;
	
	if (TActor* Result = Cast<TActor>(Actor))
		return Result;
	
	return nullptr;
}
