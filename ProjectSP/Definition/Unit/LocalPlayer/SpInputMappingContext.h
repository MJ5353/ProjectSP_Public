#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "UObject/Object.h"
#include "SpInputMappingContext.generated.h"

// ==================================================

USTRUCT()
struct FSpInputMappingContext
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	bool bShouldActivateAutomatically = true;

	UPROPERTY(EditDefaultsOnly, Category="MJ - Setting")
	TObjectPtr<const UInputMappingContext> Context = nullptr;
};
