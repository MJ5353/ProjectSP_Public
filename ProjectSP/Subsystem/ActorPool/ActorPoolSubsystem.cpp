#include "ActorPoolSubsystem.h"
#include "SpActorPoolDefine.h"
#include "Engine/World.h"
#include "SpPoolableActor.h"

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

	// 생성 완료와 게임플레이 활성화를 의도적으로 분리
	// 초기 게임 시작 대기 중 유닛은 비활성 상태이며
	// UnitData가 server로부터 복제되어야만 클라이언트 표현을 준비할 수 있다.
	SetActiveActor(Actor, false);
	
	Info->bIsPendingSpawn = false;
}

void UActorPoolSubsystem::ActivateActor(AActor* Actor)
{
	FSpActorPoolInfo* Info = SpawnedActors.Find(Actor);
	if (!Info || Info->bIsInPool || Info->bIsPendingSpawn)
		return;

	if (ISpPoolableActor* PoolableActor = Cast<ISpPoolableActor>(Actor))
		PoolableActor->OnSpawn();
	
	SetActiveActor(Actor, true);
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

	SetActiveActor(Actor, false);

	Info->bIsInPool = true;
	Bucket->Actors.Add(Actor);
}

// private

void UActorPoolSubsystem::SetActiveActor(AActor* Actor, bool bActive)
{
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
