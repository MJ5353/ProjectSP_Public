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

protected:
	UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadWrite, Category = "MJ - Runtime")
	TSet<UActorComponent*> PendingPresentationListeners;
	
	// 클라이언트 유닛이면 true, data가 오기 전까지 unit을 hide 한다.
	bool bPresentationInitialized = false;
	
	// 현재 표현 세션의 유닛 UID
	uint32 PresentationUnitUid = 0;

	// 필수 Listener가 모두 준비됐는지
	bool bPresentationReady = false;

public:	
	void InitializePresentation_Client();
	void ApplyUnitData_Client(const FSpUnitData& UnitData);
	void ReportPresentationReady_Client(const UActorComponent* Listener, uint32 UnitUid);
	void ReportPresentationReadyToGameState_Client();

private:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void BeginPresentation_Client(const FSpUnitData& UnitData);
	void CompletePresentation_Client();
	void StopPresentation_Client();
	
	// get
	ASpUnit* GetOwnerUnitChecked() const;
};
