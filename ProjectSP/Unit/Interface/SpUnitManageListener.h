#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "SpUnitManageListener.generated.h"

// ==================================================

UINTERFACE(MinimalAPI)
class USpUnitManageListener : public UInterface
{
	GENERATED_BODY()
};

class PROJECTSP_API ISpUnitManageListener
{
	GENERATED_BODY()

public:
	virtual int32 GetUnitManagePriority() const { return 0; }
	virtual void OnInitUnit() {}
	virtual void OnClearUnit() {}
	virtual void OnUnitActive(bool bActive) {}
	virtual void OnUnitPlayable(bool bPlayable) {}
	virtual void OnUnitStateChanged(FGameplayTag StateTag, bool bAdded) {}
};
