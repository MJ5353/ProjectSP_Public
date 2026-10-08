#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "SpUserWidget.generated.h"

class ASpUnit;

// ==================================================

UCLASS()
class PROJECTSP_API USpUserWidget : public UCommonUserWidget
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(Transient, VisibleAnywhere, Category="MJ - Runtime")
	TWeakObjectPtr<ASpUnit> OwnerUnit;

public:
	UFUNCTION(BlueprintCallable, Category="MJ - Attribute")
	virtual void SetOwnerUnit(ASpUnit* InOwnerUnit);
};

