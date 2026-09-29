#pragma once

#include "CoreMinimal.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpUnitStimuliSourceComponent.generated.h"

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitStimuliSourceComponent : public UAIPerceptionStimuliSourceComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

public:
	virtual void OnInitUnit() override;
	virtual void OnClearUnit() override;
	virtual void OnUnitPlayable(bool bPlayable) override;
};
