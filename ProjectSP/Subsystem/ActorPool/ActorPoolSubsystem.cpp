#include "ActorPoolSubsystem.h"
#include "SpActorPoolDefine.h"
#include "Engine/World.h"
#include "SpPoolableActor.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

void UActorPoolSubsystem::Deinitialize()
{
	Super::Deinitialize();
	
	InactiveActors.Reset();
	SpawnedActors.Reset();
}

UActorPoolSubsystem* UActorPoolSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UActorPoolSubsystem>() : nullptr;
}

// spawn

AActor* UActorPoolSubsystem::SpawnActor_Instant(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParams)
{
	FActorSpawnParameters SpawnParamsForDeferred = SpawnParams;
	
	AActor* Actor = BeginSpawnActor_Deferred(PoolKey, SpawnTransform, SpawnParamsForDeferred);
	if (!Actor)
		return nullptr;

	FinishSpawnActor_Deferred(Actor, SpawnTransform);
	ActivateActor(Actor); // 즉시 activate
	
	return Actor;
}

AActor* UActorPoolSubsystem::BeginSpawnActor_Deferred(const FSpActorPoolKey& PoolKey, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParams)
{
	FActorPoolBucket& Bucket = InactiveActors.FindOrAdd(PoolKey);
	if (AActor* Actor = Bucket.Pop())
	{
		FSpActorPoolInfo& Info = SpawnedActors.FindChecked(Actor);
		check(Info.bIsInPool);

		Info.bIsInPool = false;
		Info.bIsPendingSpawn = true;
		Info.bNeedsFinishSpawning = false;
		
		Actor->SetOwner(SpawnParams.Owner);
		Actor->SetInstigator(SpawnParams.Instigator);

		return Actor;
	}

	UWorld* World = GetWorld();
	if (!World)
		return nullptr;

	SpawnParams.bDeferConstruction = true;
	
	AActor* Actor = World->SpawnActor<AActor>(PoolKey.ActorClass, SpawnTransform, SpawnParams);
	if (!Actor)
		return nullptr;
	
	FSpActorPoolInfo Info;
	Info.PoolKey = PoolKey;
	Info.bIsPendingSpawn = true;
	Info.bNeedsFinishSpawning = true;
	
	// 파괴 시 풀 정보도 정리하도록 델리게이트를 연결한다.
	Actor->OnDestroyed.AddDynamic(this, &UActorPoolSubsystem::HandlePooledActorDestroyed);
	
	// 관리 대상으로 추가
	SpawnedActors.Add(Actor, Info);
	return Actor;
}

// after spawn

void UActorPoolSubsystem::FinishSpawnActor_Deferred(AActor* Actor, const FTransform& Transform)
{
	FSpActorPoolInfo* Info = SpawnedActors.Find(Actor);
	if (!Info || Info->bIsInPool || !Info->bIsPendingSpawn)
		return;

	if (Info->bNeedsFinishSpawning)
	{
		Actor->FinishSpawning(Transform);
		Info->bNeedsFinishSpawning = false;
		
		if (ISpPoolableActor* PoolableActor = Cast<ISpPoolableActor>(Actor))
			PoolableActor->OnCreate();
	}
	else
	{
		Actor->SetActorTransform(Transform);
	}

	// 일반 풀 Actor만 Pool이 표현과 Tick을 관리한다.
	// Unit은 SpawnSubsystem에서 SetUnitActive_Server로 상태를 설정한다.
	SetPoolManagedActorActive(Actor, false);
	
	Info->bIsPendingSpawn = false;
}

void UActorPoolSubsystem::ActivateActor(AActor* Actor)
{
	FSpActorPoolInfo* Info = SpawnedActors.Find(Actor);
	if (!Info || Info->bIsInPool || Info->bIsPendingSpawn)
		return;

	if (ISpPoolableActor* PoolableActor = Cast<ISpPoolableActor>(Actor))
		PoolableActor->OnSpawn();
	
	SetPoolManagedActorActive(Actor, true);
}

void UActorPoolSubsystem::ReturnActor(AActor* Actor)
{
	FSpActorPoolInfo* Info = SpawnedActors.Find(Actor);
	if (!Info || Info->bIsInPool || Info->bIsPendingSpawn)
		return;
	
	FActorPoolBucket* Bucket = InactiveActors.Find(Info->PoolKey);
	if (!Bucket)
		return;

	if (ISpPoolableActor* PoolableActor = Cast<ISpPoolableActor>(Actor))
		PoolableActor->OnReturn();

	SetPoolManagedActorActive(Actor, false);

	Info->bIsInPool = true;
	Bucket->Actors.Add(Actor);
}

// private

void UActorPoolSubsystem::SetPoolManagedActorActive(AActor* Actor, bool bActive)
{
	if (Actor->IsA<ASpUnit>())
		return;

	Actor->SetActorHiddenInGame(!bActive);
	Actor->SetActorTickEnabled(bActive);
}

void UActorPoolSubsystem::HandlePooledActorDestroyed(AActor* Actor)
{
	if (!Actor)
		return;
	
	ISpPoolableActor* PoolableActor = Cast<ISpPoolableActor>(Actor);
	if (PoolableActor)
		PoolableActor->OnReturn();
	
	FSpActorPoolInfo Info;
	if (!SpawnedActors.RemoveAndCopyValue(Actor, Info))
		return;
	
	if (FActorPoolBucket* Bucket = InactiveActors.Find(Info.PoolKey))
		Bucket->Actors.RemoveSingleSwap(Actor);

	if (PoolableActor)
		PoolableActor->OnDestroy();
	
	UWorld* World = GetWorld();
	
	if (!World || !World->bIsTearingDown)
		ensureMsgf(false, TEXT("Pooled Actor was destroyed outside ActorPoolSubsystem: %s"), *GetNameSafe(Actor));
}
