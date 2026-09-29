#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpTargetQueryTypes.h"
#include "SpTargetCollectorLibrary.generated.h"

class ASpUnit;
class UWorld;
struct FGenericTeamId;

// ==================================================

/**
 * 등록된 Unit 중 Query의 관계와 범위 조건을 만족하는 대상을 수집한다.
 * 실제 피해나 GameplayEffect 적용은 호출자가 담당한다.
 */
UCLASS()
class PROJECTSP_API USpTargetCollectorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="MJ - Targeting")
	static bool CollectTargets(const ASpUnit* Source, const FSpTargetQuery& Query, TArray<ASpUnit*>& OutTargets);
	
	// field는 source unit 사망 뒤에도 보존된 정보로 target을 찾는다.
	static bool CollectTargetsForField(const UWorld* World, uint32 SourceUnitUid, FGenericTeamId SourceTeamId, const FSpTargetQuery& Query, TArray<ASpUnit*>& OutTargets);
};
