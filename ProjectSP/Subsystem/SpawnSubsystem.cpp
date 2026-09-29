#include "SpawnSubsystem.h"
#include "ActorPool/ActorPoolSubsystem.h"
#include "Engine/World.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/GameMode/SpMapDefinition.h"
#include "ProjectSP/Definition/Unit/AI/SpAIUnitDefinition.h"
#include "ProjectSP/Unit/SpAIUnit.h"
#include "ProjectSP/Unit/SpPlayerUnit.h"
#include "ProjectSP/Unit/SpUnit.h"
#include "ProjectSP/Unit/AI/SpAIController.h"
#include "ProjectSP/Unit/Component/SpUnitClientGatewayComponent.h"
#include "ProjectSP/Unit/Component/SpUnitServerGatewayComponent.h"
#include "ProjectSP/Unit/Define/SpTeam.h"

// ==================================================

USpawnSubsystem* USpawnSubsystem::Get(const UWorld* World)
{
	if (!World)
		return nullptr;
	
	if (USpawnSubsystem* SpawnSubsystem = World->GetSubsystem<USpawnSubsystem>())
		return SpawnSubsystem;
	
	return nullptr;
}

void USpawnSubsystem::SpawnMapUnits(const USpMapDefinition* MapDefinition, TArray<ASpUnit*>* OutSpawnedUnits, const bool bActivateImmediately)
{
	if (!MapDefinition || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
		return;

	TArray<FTransform> SpawnTransforms;
	const auto SpawnGroups = [this, &SpawnTransforms, OutSpawnedUnits, bActivateImmediately](const TArray<FSpawnGroupData>& SpawnGroups, const FGenericTeamId& TeamId)
	{
		for (const FSpawnGroupData& GroupData : SpawnGroups)
		{
			GetSpreadLocation(GroupData, SpawnTransforms);

			for (const FTransform& Transform : SpawnTransforms)
			{
				FActorSpawnParameters Param = FActorSpawnParameters();
				if (ASpAIUnit* SpawnedUnit = SpawnAIUnit(GroupData.UnitDefinition, Transform, Param, TeamId, bActivateImmediately))
				{
					if (OutSpawnedUnits)
						OutSpawnedUnits->Add(SpawnedUnit);
				}
			}
		}
	};

	SpawnGroups(MapDefinition->EnemySpawnGroups, FGenericTeamId(SpTeam::EnemyId));
	SpawnGroups(MapDefinition->OtherSpawnGroups, FGenericTeamId::NoTeam);
}

void USpawnSubsystem::ActivateUnit(ASpUnit* Unit, const bool bKeepAlwaysRelevant)
{
	UWorld* World = GetWorld();
	
	if (!Unit || !World || World->GetNetMode() == NM_Client)
		return;

	if (USpUnitServerGatewayComponent* ServerGateway = Unit->GetServerGateway())
		ServerGateway->ActivateUnit_Server();

	// 로비에서 맵 유닛 presentation을 준비하는 동안에는 모든 클라이언트가 유닛을 받을 수 있어야 한다.
	if (!bKeepAlwaysRelevant)
		ReleaseInitialRelevancy(Unit);
	else
		Unit->ForceNetUpdate();
}

void USpawnSubsystem::ReleaseInitialRelevancy(ASpUnit* Unit)
{
	UWorld* World = GetWorld();
	if (!Unit || !World || World->GetNetMode() == NM_Client)
		return;

	// 초기 presentation 대상 전송이 끝난 뒤에만 거리 기반 relevancy로 되돌린다.
	Unit->bAlwaysRelevant = false;
	Unit->ForceNetUpdate();
}

// spawn unit

ASpAIUnit* USpawnSubsystem::SpawnAIUnit(USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParameters, const FGenericTeamId& TeamId, const bool bActivateImmediately)
{
	if (!UnitDefinition || !UnitDefinition->UnitClass || !UnitDefinition->UnitClass->IsChildOf(ASpAIUnit::StaticClass()))
		return nullptr;

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
		return nullptr;

	const bool bEnemyUnit = TeamId.GetId() == SpTeam::EnemyId;
	if (bEnemyUnit)
	{
		for (auto It = ActiveEnemyUnits.CreateIterator(); It; ++It)
		{
			const ASpAIUnit* Unit = (*It).Get();
			
			// valid 하지 않은 unit 제거
			if (!IsValid(Unit) || !Unit->HasValidUnitData() || Unit->GetGenericTeamId().GetId() != SpTeam::EnemyId)
				It.RemoveCurrent();
		}

		// 활성화할 적 유닛 수를 제한
		if (ActiveEnemyUnits.Num() >= MaxConcurrentEnemyUnits)
			return nullptr;
	}

	UActorPoolSubsystem* ActorPoolSubsystem = World->GetSubsystem<UActorPoolSubsystem>();
	if (!ActorPoolSubsystem)
		return nullptr;

	const FSpActorPoolKey Key = {UnitDefinition->UnitClass, FName(*UnitDefinition->GetPathName())};

	ASpAIUnit* SpawnedUnit = ActorPoolSubsystem->BeginSpawnActor_Deferred<ASpAIUnit>(Key, SpawnTransform, SpawnParameters);
	if (!SpawnedUnit)
		return nullptr;

	USpUnitServerGatewayComponent* ServerGateway = SpawnedUnit->GetServerGateway();
	if (!ServerGateway)
		return nullptr;

	FSpUnitData UnitData(AllocateUnitUid(), UnitDefinition, TeamId.GetId());
	UnitData.bRevealWhenPresentationReady = bActivateImmediately;
	ServerGateway->ApplyUnitData_Server(UnitData);
	
	if (bEnemyUnit)
		ActiveEnemyUnits.Add(SpawnedUnit);
	
	bool bSpawnAIController = false;
	USpAIDefinition* AIDefinition = UnitDefinition->AIDefinition;
	
	if (AIDefinition && AIDefinition->AIControllerClass != nullptr)
	{
		bSpawnAIController = true;
		SpawnedUnit->AIControllerClass = AIDefinition->AIControllerClass;
	}

	ActorPoolSubsystem->FinishSpawnActor_Deferred(SpawnedUnit, SpawnTransform);
	PrepareSpawnedUnit_Server(SpawnedUnit, ServerGateway);
	
	if (bSpawnAIController && !SpawnedUnit->GetController())
		SpawnedUnit->SpawnDefaultController();
	
	if (ASpAIController* Controller = SpawnedUnit->GetController<ASpAIController>())
		Controller->SetDefinition(AIDefinition);

	if (bActivateImmediately)
		ActivateUnit(SpawnedUnit);
	
	return SpawnedUnit;
}

ASpPlayerUnit* USpawnSubsystem::SpawnPlayerUnit(USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, const FActorSpawnParameters& SpawnParameters, const FGenericTeamId& TeamId, const bool bActivateImmediately)
{
	if (!UnitDefinition || !UnitDefinition->UnitClass || !UnitDefinition->UnitClass->IsChildOf(ASpPlayerUnit::StaticClass()))
		return nullptr;

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
		return nullptr;

	FActorSpawnParameters DeferredSpawnParameters = SpawnParameters;
	DeferredSpawnParameters.bDeferConstruction = true;

	ASpPlayerUnit* SpawnedUnit = World->SpawnActor<ASpPlayerUnit>(UnitDefinition->UnitClass, SpawnTransform, DeferredSpawnParameters);
	if (!SpawnedUnit)
		return nullptr;

	USpUnitServerGatewayComponent* ServerGateway = SpawnedUnit->GetServerGateway();
	if (!ServerGateway)
		return nullptr;

	ServerGateway->ApplyUnitData_Server(FSpUnitData(AllocateUnitUid(), UnitDefinition, TeamId.GetId()));
	SpawnedUnit->FinishSpawning(SpawnTransform);

	// 초기 Presentation 완료 전에는 표현을 활성화하지 않는다.
	PrepareSpawnedUnit_Server(SpawnedUnit, ServerGateway);

	if (bActivateImmediately)
		ActivateUnit(SpawnedUnit);

	return SpawnedUnit;
}

// sync

void USpawnSubsystem::PrepareSpawnedUnit_Server(ASpUnit* SpawnedUnit, const USpUnitServerGatewayComponent* ServerGateway)
{
	check(SpawnedUnit);
	check(ServerGateway);
	
	// Unit의 숨김, Tick, 충돌, 등록 상태는 Pool이 아닌 Unit 라이프사이클로 단일 관리
	SpawnedUnit->SetUnitActive_Server(false, true);
	SpawnedUnit->bAlwaysRelevant = true;
	
	ServerGateway->PrepareUnit_Server();

	// Listen Server의 호스트는 자신의 UnitData를 네트워크 복제로 다시 수신하지 않음
	if (GetWorld()->GetNetMode() == NM_ListenServer)
	{
		if (USpUnitClientGatewayComponent* ClientGateway = SpawnedUnit->GetClientGateway())
			ClientGateway->ApplyUnitData_Client(SpawnedUnit->GetUnitData());
	}

	SpawnedUnit->ForceNetUpdate();
}

// get

uint32 USpawnSubsystem::AllocateUnitUid()
{
	const uint32 UnitUid = NextUnitUid;
	++NextUnitUid;

	if (NextUnitUid == FSpUnitData::InvalidUnitUid)
		++NextUnitUid;

	return UnitUid;
}

void USpawnSubsystem::GetSpreadLocation(const FSpawnGroupData& GroupData, TArray<FTransform>& SpawnTransforms)
{
	SpawnTransforms.Reset(GroupData.Count);

	if (!GroupData.UnitDefinition || !GroupData.UnitDefinition->UnitClass)
		return;

	const FTransform Origin = GroupData.SpawnTransform;

	for (uint8 i = 0; i < GroupData.Count; ++i)
	{
		const float RandomLength = GroupData.MaxRadius * FMath::Sqrt(FMath::FRand());
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);

		const FVector LocalOffset(FMath::Cos(Angle) * RandomLength, FMath::Sin(Angle) * RandomLength, 0.f);
		const FVector WorldOffset = Origin.TransformVectorNoScale(LocalOffset);

		FTransform Transform = FTransform(Origin.GetLocation() + WorldOffset);
		SpawnTransforms.Add(Transform);
	}
}
