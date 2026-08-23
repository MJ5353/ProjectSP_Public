#include "SpEnemyAIController.h"
#include "DrawDebugHelpers.h"
#include "FSpAIAbilityOption.h"
#include "HAL/IConsoleManager.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/Definition/Unit/AI/SpEnemyAIDefinition.h"
#include "ProjectSP/GameFramework/SpGameplayTags.h"
#include "ProjectSP/Unit/Component/Common/SpUnitAggroComponent.h"
#include "ProjectSP/Unit/SpUnit.h"

// ==================================================

#if !UE_BUILD_SHIPPING
namespace SpEnemyAI::Debug
{
	static int32 DrawChaseRadius = 0;
	static FAutoConsoleVariableRef CVarDrawChaseRadius(
		TEXT("sp.ai.DebugDrawChaseRadius"), 
		DrawChaseRadius, 
		TEXT("Draw the enemy AI chase radius. 0: Off, 1: On"), 
		ECVF_Cheat);
}
#endif

// ==================================================

ASpEnemyAIController::ASpEnemyAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*AIPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 70.0f;
	SightConfig->SetMaxAge(2.0f);

	// 처음에는 팀 판정 문제를 피하기 위해 모두 감지.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	AIPerceptionComponent->ConfigureSense(*SightConfig);
	AIPerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
	AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
}

// virtual

void ASpEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
	if (SpEnemyAI::Debug::DrawChaseRadius != 0 && EnemyAIDefinition && EnemyAIDefinition->ChaseRadius > 0.0f)
		DrawDebugCircle(GetWorld(), HomeLocation, EnemyAIDefinition->ChaseRadius, 
			64, FColor::Cyan, false, 
			0.0f, 0, 3.0f, 
			FVector::ForwardVector, FVector::RightVector, false);
#endif
}

void ASpEnemyAIController::OnPossess(APawn* InPawn)
{
	UnbindAggroComponent();

	Super::OnPossess(InPawn);
	BindAggroComponent();
	RefreshCombatTarget();
}

void ASpEnemyAIController::OnUnPossess()
{
	UnbindAggroComponent();

	Super::OnUnPossess();
}

void ASpEnemyAIController::OnUnitActive(bool bActive)
{
	Super::OnUnitActive(bActive);

	if (bActive)
		return;

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetActor);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::HomeLocation);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::PatrolLocation);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetFacingRange);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::CanChaseTarget);
	}
}

// aggro

void ASpEnemyAIController::BindAggroComponent()
{
	ASpUnit* Unit = GetPawn<ASpUnit>();

	USpUnitAggroComponent* AggroComponent = Unit ? Unit->GetUnitComponent<USpUnitAggroComponent>() : nullptr;
	if (!AggroComponent || AggroChangedHandle.IsValid())
		return;

	BoundAggroComponent = AggroComponent;
	AggroChangedHandle = AggroComponent->OnAggroChanged.AddUObject(this, &ThisClass::HandleAggroChanged);
}

void ASpEnemyAIController::UnbindAggroComponent()
{
	if (BoundAggroComponent.IsValid())
		BoundAggroComponent->OnAggroChanged.Remove(AggroChangedHandle);

	AggroChangedHandle.Reset();
	BoundAggroComponent.Reset();
}

void ASpEnemyAIController::HandleAggroChanged()
{
	RefreshCombatTarget();
}

bool ASpEnemyAIController::IsAggroTargetEligible(const ASpUnit* Target) const
{
	if (!Target)
		return false;

	return CheckNeedChase(Target->GetActorLocation());
}

// set

void ASpEnemyAIController::SetDefinition(USpAIDefinition* Definition)
{
	USpEnemyAIDefinition* NewDefinition = Cast<USpEnemyAIDefinition>(Definition);
	if (!NewDefinition)
		return;

	EnemyAIDefinition = NewDefinition;

	if (APawn* ControlledPawn = GetPawn())
		HomeLocation = ControlledPawn->GetActorLocation();

	Super::SetDefinition(Definition);
}

// ufunction

void ASpEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent || !IsValid(Actor))
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return;

	if (ASpUnit* TargetUnit = Cast<ASpUnit>(Actor))
	{
		if (!TargetUnit->IsAttackable(Unit))
			return;
	}

	if (USpUnitAggroComponent* AggroComponent = Unit->GetUnitComponent<USpUnitAggroComponent>())
	{
		AggroComponent->SetTargetSensed(Actor, Stimulus.WasSuccessfullySensed());
		RefreshCombatTarget();
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
		BlackboardComponent->SetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor, Actor);
	else if (BlackboardComponent->GetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor) == Actor)
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetActor);
}

bool ASpEnemyAIController::TryGetActivatableAbility(FGameplayTag& OutTag, bool& bNeedTarget, float& OutExecuteRange)
{
	OutTag = FGameplayTag();

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return false;

	USpAbilitySystemComponent* SpASC = Unit->GetSpAbilitySystemComponent();
	if (!SpASC)
		return false;

	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
		return false;

	if (!EnemyAIDefinition)
		return false;

	UObject* TargetObject = BlackboardComponent->GetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor);
	bool bValidTarget = TargetObject != nullptr;

	AActor* TargetActor = bValidTarget ? Cast<AActor>(TargetObject) : nullptr;
	bValidTarget = TargetActor != nullptr;

	for (const FSpAIAbilityOption& Option : EnemyAIDefinition->AbilityOptions)
	{
		if (SpASC->CanActivateAbilityByTag(Option.AbilityTag) == false)
			continue;

		FGameplayTag NeedTargetTag = FSpGameplayTags::Get().AbilityConditionTag_NeedTarget;
		bNeedTarget = SpASC->HasAbilityAssetTag(Option.AbilityTag, NeedTargetTag);

		if (bNeedTarget && !bValidTarget)
			continue;

		if (bNeedTarget)
			OutExecuteRange = FMath::Max(Option.ExecuteRange, 0.0f);

		OutTag = Option.AbilityTag;
		return true;
	}
	return false;
}

// check

bool ASpEnemyAIController::CheckDistanceFromHome(const FVector& TargetLocation, const float Distance) const
{
	float DistanceSqrt = FVector::DistSquared(TargetLocation, HomeLocation);
	float HomeRadiusSqrt = FMath::Square(Distance);

	return DistanceSqrt <= HomeRadiusSqrt;
}

bool ASpEnemyAIController::CheckNeedChase(const FVector& TargetLocation) const
{
	if (!EnemyAIDefinition)
		return false;

	return CheckDistanceFromHome(TargetLocation, EnemyAIDefinition->ChaseRadius);
}

bool ASpEnemyAIController::CheckNeedReturn(FVector& OutLocation)
{
	AActor* OwnerActor = GetPawn();
	if (!OwnerActor)
		return false;

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	if (CheckDistanceFromHome(OwnerLocation, EnemyAIDefinition->HomeRadius))
		return false;

	OutLocation = HomeLocation;
	return true;
}

// task

bool ASpEnemyAIController::TryGetRandomLocationInHomeRange(FVector& OutLocation)
{
	if (EnemyAIDefinition->HomeRadius <= 0.f)
		return false;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return false;

	FVector OwnerLocation = Unit->GetActorLocation();

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
	if (!NavigationSystem)
		return false;

	FNavLocation NavLocation;
	if (!NavigationSystem->GetRandomReachablePointInRadius(HomeLocation, EnemyAIDefinition->HomeRadius, NavLocation))
		return false;

	OutLocation = NavLocation.Location;
	return true;
}

void ASpEnemyAIController::RefreshCombatTarget()
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	ASpUnit* Unit = GetPawn<ASpUnit>();
	USpUnitAggroComponent* AggroComponent = Unit ? Unit->GetUnitComponent<USpUnitAggroComponent>() : nullptr;

	if (!BlackboardComponent || !AggroComponent)
		return;

	AActor* CurrentTarget = Cast<AActor>(BlackboardComponent->GetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor));
	auto FIsTargetEligible = [this](const ASpUnit* Candidate)
	{
		return IsAggroTargetEligible(Candidate);
	};

	if (ASpUnit* BestTarget = AggroComponent->GetBestTarget(CurrentTarget, 1.25f, FIsTargetEligible))
		BlackboardComponent->SetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor, BestTarget);
	else
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetActor);
}
