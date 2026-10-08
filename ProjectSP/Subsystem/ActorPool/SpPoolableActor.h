#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpPoolableActor.generated.h"

// ==================================================

UINTERFACE()
class USpPoolableActor : public UInterface
{
	GENERATED_BODY()
};

// ------------------------------------------------

class PROJECTSP_API ISpPoolableActor
{
	GENERATED_BODY()

public:
	virtual void Push() = 0; 
	
	// 생성시, 파괴시
	virtual void OnCreate() {}
	virtual void OnDestroy() {}
	
	// pool에서 꺼낼 때, 넣을 때
	virtual void OnSpawn() {}
	virtual void OnReturn() {}
};
