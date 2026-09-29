#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ProjectSP/Unit/Define/SpUnitData.h"
#include "SpUnitClientListener.generated.h"

// ==================================================

UINTERFACE(MinimalAPI)
class USpUnitClientListener : public UInterface
{
	GENERATED_BODY()
};

class PROJECTSP_API ISpUnitClientListener
{
	GENERATED_BODY()

public:
	virtual bool IsClientPresentationRequired() const { return false; }
	virtual void PrepareClientPresentation(const FSpUnitData& UnitData) {}
	virtual void StopClientPresentation() {}
};
