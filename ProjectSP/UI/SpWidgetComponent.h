#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "SpWidgetComponent.generated.h"

class ASpUnit;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	
public:
	virtual void SetOwner(ASpUnit* InOwnerUnit);
};

