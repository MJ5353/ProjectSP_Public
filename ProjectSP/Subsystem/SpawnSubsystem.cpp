#include "SpawnSubsystem.h"
#include "ActorPool/ActorPoolSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "ProjectSP/Definition/Unit/SpUnitDefinition.h"
#include "ProjectSP/Definition/GameMode/SpMapDefinition.h"
#include "ProjectSP/Definition/Unit/AI/SpAIUnitDefinition.h"
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
				if (ASpUnit* SpawnedUnit = SpawnUnit(GroupData.UnitDefinition, Transform, Param, TeamId, bActivateImmediately))
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

ASpUnit* USpawnSubsystem::SpawnUnit(USpUnitDefinition* UnitDefinition, const FTransform& SpawnTransform, FActorSpawnParameters& SpawnParameters, const FGenericTeamId& TeamId, const bool bActivateImmediately)
{
	if (!UnitDefinition || !UnitDefinition->UnitClass)
		return nullptr;

	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
		return nullptr;

	UActorPoolSubsystem* ActorPoolSubsystem = World->GetSubsystem<UActorPoolSubsystem>();
	if (!ActorPoolSubsystem)
		return nullptr;

	const FSpActorPoolKey Key = {UnitDefinition->UnitClass, FName(UnitDefinition->Name)};

	ASpUnit* SpawnedUnit = ActorPoolSubsystem->BeginSpawnActor_Deferred<ASpUnit>(Key, SpawnTransform, SpawnParameters);
	if (!SpawnedUnit)
		return nullptr;

	USpUnitServerGatewayComponent* ServerGateway = SpawnedUnit->GetServerGateway();
	if (!ServerGateway)
		return nullptr;

	FSpUnitData UnitData = FSpUnitData(AllocateUnitUid(), UnitDefinition, TeamId.GetId());
	ServerGateway->PrepareUnit_ServerOnly(UnitData);
	
	bool bSpawnAIController = false;
	USpAIDefinition* AIDefinition = UnitDefinition->AIDefinition;
	
	if (AIDefinition && AIDefinition->AIControllerClass != nullptr)
	{
		bSpawnAIController = true;
		SpawnedUnit->AIControllerClass = AIDefinition->AIControllerClass;
	}

	ActorPoolSubsystem->FinishSpawnActor_Deferred(SpawnedUnit, SpawnTransform);

	// 숨김 + 무충돌 상태에서도 초기 UnitData를 클라이언트에 보낸다.
	SpawnedUnit->bAlwaysRelevant = true;
	ServerGateway->PrepareUnit_ServerOnly(UnitData);
	
	// Listen Server의 호스트는 자신의 UnitData를 네트워크 복제로 다시 수신하지 않음
	if (World->GetNetMode() == NM_ListenServer)
	{
		if (USpUnitClientGatewayComponent* ClientGateway = SpawnedUnit->GetClientGateway())
			ClientGateway->ApplyUnitData_ClientOnly(SpawnedUnit->GetUnitData());
	}
	
	if (bSpawnAIController && !SpawnedUnit->GetController())
		SpawnedUnit->SpawnDefaultController();
	
	if (ASpAIController* Controller = SpawnedUnit->GetController<ASpAIController>())
		Controller->SetDefinition(AIDefinition);

	if (bActivateImmediately)
		ActivateUnit(SpawnedUnit);
	
	return SpawnedUnit;
}

void USpawnSubsystem::ActivateUnit(ASpUnit* Unit)
{
	UWorld* World = GetWorld();
	
	if (!Unit || !World || World->GetNetMode() == NM_Client)
		return;

	if (USpUnitServerGatewayComponent* ServerGateway = Unit->GetServerGateway())
		ServerGateway->ActivateUnit_ServerOnly();

	if (UActorPoolSubsystem* ActorPoolSubsystem = World->GetSubsystem<UActorPoolSubsystem>())
		ActorPoolSubsystem->ActivateActor(Unit);

	// 초기 복제가 끝났으므로 이후에는 원래 거리 기반 relevancy를 사용한다.
	Unit->bAlwaysRelevant = false;
	Unit->ForceNetUpdate();
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

	const ASpUnit* UnitCDO = GroupData.UnitDefinition->UnitClass->GetDefaultObject<ASpUnit>();
	if (!UnitCDO || !UnitCDO->GetCapsuleComponent())
		return;

	const FTransform Origin = GroupData.SpawnTransform;
	const float HalfHeight = UnitCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

	for (uint8 i = 0; i < GroupData.Count; ++i)
	{
		const float RandomLength = GroupData.MaxRadius * FMath::Sqrt(FMath::FRand());
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);

		const FVector LocalOffset(FMath::Cos(Angle) * RandomLength, FMath::Sin(Angle) * RandomLength, 0.f);
		const FVector WorldOffset = Origin.TransformVectorNoScale(LocalOffset);

		FTransform Transform = Origin;
		Transform.SetLocation(Origin.GetLocation() + WorldOffset + FVector(0.f, 0.f, HalfHeight));

		SpawnTransforms.Add(Transform);
	}
}
