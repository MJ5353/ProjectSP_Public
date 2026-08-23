#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ProjectSP/Unit/Define/SpUnitData.h"
#include "SpUnitClientGatewayComponent.generated.h"

class ASpUnit;

// ==================================================

// 유닛의 클라이언트 표현용 comp - camera, input 등
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitClientGatewayComponent : public UActorComponent
{
	GENERATED_BODY()

	// 클라이언트 유닛이면 true, data가 오기 전까지 unit을 hide 한다.
	bool bPresentationInitialized = false;
	
	// 유효한 data로 presentation을 시작했는지
	bool bPresentationStarted = false;

public:
	void InitializePresentation_ClientOnly();
	void ApplyUnitData_ClientOnly(const FSpUnitData& UnitData);

private:
	bool NotifyPresentationPrepared_ClientOnly(const FSpUnitData& UnitData) const;
	void StopPresentation_ClientOnly();
	
	// get
	ASpUnit* GetOwnerUnitChecked() const;
};
