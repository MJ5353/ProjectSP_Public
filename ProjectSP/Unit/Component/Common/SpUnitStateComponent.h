#pragma once

#include "CoreMinimal.h"
#include "Components/PawnComponent.h"
#include "ProjectSP/Unit/Interface/SpUnitManageListener.h"
#include "SpUnitStateComponent.generated.h"

struct FGameplayEventData;

// ==================================================

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTSP_API USpUnitStateComponent : public UPawnComponent, public ISpUnitManageListener
{
	GENERATED_BODY()

public:
	bool IsOnDead = false;
	
private:
	FDelegateHandle DeadEventHandle;

public:
	virtual int32 GetUnitManagePriority() const override { return -100; }
	virtual void OnInitUnit() override;
	virtual void OnClearUnit() override;

private:
	void RegisterDeadEvent();
	void UnregisterDeadEvent();

	void HandleGameplayEvent_Dead(const FGameplayEventData* Payload);
};
