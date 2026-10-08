#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpUnitServerListener.generated.h"

// ==================================================

UINTERFACE(MinimalAPI)
class USpUnitServerListener : public UInterface
{
	GENERATED_BODY()
};

// 서버에서만 실행되는 유닛 생명주기 콜백
class PROJECTSP_API ISpUnitServerListener
{
	GENERATED_BODY()

public:
	virtual void OnServerUnitPrepared(const FSpUnitData& UnitData) {}
	virtual void OnServerUnitActivated(const FSpUnitData& UnitData) {}
	virtual void OnServerUnitReturned() {}
};
