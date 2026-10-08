#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AggroDefinition.generated.h"

// ==================================================

UCLASS()
class PROJECTSP_API UAggroDefinition : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 실제 적용 피해량에 곱하는 Threat 계수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float DamageThreatMultiplier = 1.0f;

	// 시야로 처음 발견한 적에게 부여하는 최소 Threat
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float SightInitialThreat = 1.0f;

	// Threat를 유지하는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float ThreatMemorySeconds = 5.0f;

	// 기억 시간이 지난 비가시 대상의 초당 Threat 감소 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float ThreatDecayPercentPerSecond = 0.2f;

	// 감쇠된 Threat가 이 값 이하가 되면 제거
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MJ - Setting", meta = (AllowPrivateAccess = "true"))
	float ThreatRemovalThreshold = 0.01f;
};
