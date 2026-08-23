#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpUnitServerGatewayComponent.generated.h"

class ASpUnit;
struct FSpUnitData;

// ==================================================

/**
 * 서버 측 유닛 준비, 활성화, 반환의 진입점. SpawnSubsystem만 이 컴포넌트를 호출할 것
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitServerGatewayComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	void PrepareUnit_ServerOnly(const FSpUnitData& UnitData);
	void ActivateUnit_ServerOnly();
	void ReturnUnit_ServerOnly();

private:
	// notify
	void NotifyPrepared_ServerOnly(const FSpUnitData& UnitData) const;
	void NotifyActivated_ServerOnly(const FSpUnitData& UnitData) const;
	void NotifyReturned_ServerOnly() const;

	// get
	ASpUnit* GetOwnerUnitChecked() const;
};