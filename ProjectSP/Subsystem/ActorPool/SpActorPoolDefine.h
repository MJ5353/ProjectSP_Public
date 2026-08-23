#pragma once

#include "CoreMinimal.h"
#include "SpActorPoolDefine.generated.h"

// ==================================================

USTRUCT()
struct FSpActorPoolKey
{
	GENERATED_BODY()
	
	UPROPERTY()
	TSubclassOf<AActor> ActorClass;

	UPROPERTY()
	FName PoolKey;

	bool operator==(const FSpActorPoolKey& Other) const
	{
		return ActorClass == Other.ActorClass && PoolKey == Other.PoolKey;
	}

	friend uint32 GetTypeHash(const FSpActorPoolKey& Key)
	{
		return HashCombine(GetTypeHash(Key.ActorClass), GetTypeHash(Key.PoolKey));
	}
};

// ==================================================

USTRUCT()
struct FActorPoolBucket
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<TObjectPtr<AActor>> Actors;
	
	AActor* Pop()
	{
		return Actors.Num() > 0 ? Actors.Pop() : nullptr;
	}
	
	void Push(AActor* Actor)
	{
		Actors.Push(Actor);
	}
};

// ==================================================

USTRUCT()
struct FSpActorPoolInfo
{
	GENERATED_BODY()
	
	UPROPERTY()
	FSpActorPoolKey PoolKey;

	UPROPERTY()
	bool bIsInPool = false;

	UPROPERTY()
	bool bIsPendingSpawn = false;

	UPROPERTY()
	bool bNeedsFinishSpawning = false;
};
