#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SpPlayerCommandTypes.generated.h"

class ASpUnit;
struct FSpPlayerCommandContext_Server;

// ==================================================

UENUM()
enum class ESpPlayerCommandInputPhase : uint8
{
	Begin,
	Update,
	End,
};

// 클라는 서버에게 커서에서 만들어진 월드 Ray만 전달
// 명령 종류와 대상 판정은 서버 CommandComponent가 수행한다.
USTRUCT()
struct PROJECTSP_API FSpPlayerCommandCursorRay
{
	GENERATED_BODY()

	UPROPERTY()
	FVector_NetQuantize10 Origin;

	UPROPERTY()
	FVector_NetQuantizeNormal Direction;
};

// 클라이언트가 보내는 명령 입력 데이터. 명령 종류와 대상은 서버가 Ray를 다시 판정한다.
USTRUCT()
struct PROJECTSP_API FSpPlayerCommandInput
{
	GENERATED_BODY()

	UPROPERTY()
	uint16 CommandId = 0;

	UPROPERTY()
	ESpPlayerCommandInputPhase Phase = ESpPlayerCommandInputPhase::Begin;

	UPROPERTY()
	uint16 Sequence = 0;

	UPROPERTY()
	FSpPlayerCommandCursorRay CursorRay;
};

// ==================================================

enum class ESpPlayerCommandAbilityValidation : uint8
{
	Accepted,
	Rejected,
	EndCommand,
};

enum class ESpPlayerCommandKind : uint8
{
	Move,
	PrimaryAttack,
};

// 서버가 CursorRay를 검증한 뒤 명령 정책에 넘기는 데이터.
struct FSpPlayerCommandResolvedInput
{
	uint16 CommandId = 0;
	uint16 Sequence = 0;
	bool bHasMoveDestination = false;
	FVector MoveDestination = FVector::ZeroVector;
};

// CursorRay를 해석한 결과. 네트워크로 전송하지 않으며, Resolver와 Factory 사이에서만 사용한다.
struct FSpPlayerCommandResolution
{
	ESpPlayerCommandKind Kind = ESpPlayerCommandKind::Move;
	TWeakObjectPtr<ASpUnit> TargetUnit;
	FVector MoveDestination = FVector::ZeroVector;
};

// 네트워크로 전송하지 않는 서버 실행 전략.
class ISpPlayerCommand
{
public:
	virtual ~ISpPlayerCommand() = default;

	virtual uint16 GetId() const = 0;
	virtual bool ShouldEndOnInputEnd() const { return false; }
	virtual AActor* GetTargetActor() const { return nullptr; }
	
	virtual bool Begin(FSpPlayerCommandContext_Server& Context) = 0;
	virtual bool Update(FSpPlayerCommandContext_Server& Context, const FSpPlayerCommandResolvedInput& Input) = 0;
	virtual bool Tick(FSpPlayerCommandContext_Server& Context, float DeltaTime) = 0;
	
	virtual ESpPlayerCommandAbilityValidation ValidateAbility(FSpPlayerCommandContext_Server& Context, FGameplayTag AbilityTag) = 0;
};

// ==================================================

// 로컬 즉시 반응만 표현한다. 서버 명령과 MoveState에는 의존하지 않는다.
class ISpPlayerCommandPrediction
{
public:
	virtual ~ISpPlayerCommandPrediction() = default;

	virtual uint16 GetId() const = 0;
	virtual bool ShouldEndOnInputEnd() const { return true; }
	virtual bool Tick(float DeltaTime) = 0;
	virtual void Cancel() = 0;
};
