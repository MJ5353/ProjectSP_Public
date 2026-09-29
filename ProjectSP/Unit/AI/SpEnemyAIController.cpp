#include "SpEnemyAIController.h"
#include "DrawDebugHelpers.h"
#include "FSpAIAbilityOption.h"
#include "HAL/IConsoleManager.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"
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
	UnbindTargetChanged();

	Super::OnPossess(InPawn);
	
	BindTargetChanged();
	SyncBlackboardTarget();
	
	BindAggroComponent();
	RefreshCombatTarget();
}

void ASpEnemyAIController::OnUnPossess()
{
	UnbindAggroComponent();
	UnbindTargetChanged();

	if (ASpUnit* Unit = GetPawn<ASpUnit>())
		Unit->SetTargetActor(nullptr);

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetActor);

	Super::OnUnPossess();
}

void ASpEnemyAIController::OnUnitPlayable(bool bPlayable)
{
	Super::OnUnitPlayable(bPlayable);

	if (bPlayable)
	{
		SyncBlackboardTarget();
		return;
	}

	if (ASpUnit* Unit = GetPawn<ASpUnit>())
		Unit->SetTargetActor(nullptr);

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetActor);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::HomeLocation);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::PatrolLocation);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetFacingRange);
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::CanChaseTarget);
	}
}

FPathFollowingRequestResult ASpEnemyAIController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	const ASpUnit* Unit = GetPawn<ASpUnit>();
	const ASpUnit* Target = Unit ? Unit->GetTargetActor() : nullptr;

	if (Target && MoveRequest.IsMoveToActorRequest() && MoveRequest.GetGoalActor() == Target)
	{
		// Match the center-to-center distance used by the ability range check.
		FAIMoveRequest CombatMoveRequest = MoveRequest;
		CombatMoveRequest.SetReachTestIncludesAgentRadius(false);
		CombatMoveRequest.SetReachTestIncludesGoalRadius(false);
		
		return Super::MoveTo(CombatMoveRequest, OutPath);
	}

	return Super::MoveTo(MoveRequest, OutPath);
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

// target

void ASpEnemyAIController::BindTargetChanged()
{
	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit || TargetChangedHandle.IsValid())
		return;

	BoundTargetUnit = Unit;
	TargetChangedHandle = Unit->OnTargetChanged.AddUObject(this, &ThisClass::HandleTargetChanged);
}

void ASpEnemyAIController::UnbindTargetChanged()
{
	if (ASpUnit* Unit = BoundTargetUnit.Get())
		Unit->OnTargetChanged.Remove(TargetChangedHandle);

	TargetChangedHandle.Reset();
	BoundTargetUnit.Reset();
}

void ASpEnemyAIController::HandleTargetChanged()
{
	SyncBlackboardTarget();
}

bool ASpEnemyAIController::IsAggroTargetEligible(const ASpUnit* Target) const
{
	if (!Target)
		return false;

	return CheckNeedChase(Target->GetActorLocation());
}

void ASpEnemyAIController::SyncBlackboardTarget()
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!BlackboardComponent)
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	ASpUnit* Target = Unit ? Unit->GetTargetActor() : nullptr;
	
	if (Target)
		BlackboardComponent->SetValueAsObject(SpEnemyAI::BlackboardKeys::TargetActor, Target);
	else
		BlackboardComponent->ClearValue(SpEnemyAI::BlackboardKeys::TargetActor);
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
	SyncBlackboardTarget();
}

// ufunction

void ASpEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ASpUnit* TargetUnit = Cast<ASpUnit>(Actor);
	if (!IsValid(TargetUnit))
		return;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return;

	if (Stimulus.WasSuccessfullySensed() && !Unit->IsAttackable(TargetUnit))
		return;

	if (USpUnitAggroComponent* AggroComponent = Unit->GetUnitComponent<USpUnitAggroComponent>())
	{
		AggroComponent->SetTargetSensed(TargetUnit, Stimulus.WasSuccessfullySensed());
		RefreshCombatTarget();
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
		Unit->SetTargetActor(TargetUnit);
	else if (Unit->GetTargetActor() == TargetUnit)
		Unit->SetTargetActor(nullptr);
}

bool ASpEnemyAIController::TryGetActivatableAbility(FGameplayTag& OutTag, bool& bNeedTarget, float& OutExecuteRange)
{
	OutTag = FGameplayTag();
	bNeedTarget = false;
	OutExecuteRange = SpEnemyAI::Define::FocusRange;

	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
		return false;

	USpAbilitySystemComponent* SpASC = Unit->GetSpAbilitySystemComponent();
	if (!SpASC)
		return false;

	if (!EnemyAIDefinition)
		return false;

	const ASpUnit* Target = Unit->GetTargetActor();
	const bool bValidTarget = Unit->IsAttackable(Target);

	for (const FSpAIAbilityOption& Option : EnemyAIDefinition->AbilityOptions)
	{
		if (SpASC->CanActivateAbilityByTag(Option.AbilityTag) == false)
			continue;

		const FGameplayTag NeedTargetTag = SpGameplayTags::AbilityTargetTag_Hostile;
		const FGameplayTag NeedTargetFacingTag = SpGameplayTags::AbilityConditionTag_TargetFacing;
		const bool bOptionNeedsTarget = SpASC->HasAbilityAssetTag(Option.AbilityTag, NeedTargetTag) || SpASC->HasAbilityAssetTag(Option.AbilityTag, NeedTargetFacingTag);
		
		if (bOptionNeedsTarget && !bValidTarget)
			continue;

		float OptionExecuteRange = SpEnemyAI::Define::FocusRange;
		if (bOptionNeedsTarget)
		{
			if (!SpASC->TryGetAbilityExecuteRangeByTag(Option.AbilityTag, OptionExecuteRange))
				continue;

			const float AllowedRange = OptionExecuteRange + SpEnemyAI::Define::TargetRangeTolerance;
			if (FVector::DistSquared2D(Unit->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(AllowedRange))
			{
				if (!bNeedTarget)
				{
					bNeedTarget = true;
					OutExecuteRange = OptionExecuteRange;
				}
				continue;
			}
		}

		OutTag = Option.AbilityTag;
		bNeedTarget = bOptionNeedsTarget;
		OutExecuteRange = OptionExecuteRange;
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
	ASpUnit* Unit = GetPawn<ASpUnit>();
	if (!Unit)
	{
		SyncBlackboardTarget();
		return;
	}

	USpUnitAggroComponent* AggroComponent = Unit->GetUnitComponent<USpUnitAggroComponent>();
	if (!AggroComponent)
	{
		if (!Unit->IsAttackable(Unit->GetTargetActor()))
			Unit->SetTargetActor(nullptr);
		
		SyncBlackboardTarget();
		return;
	}

	ASpUnit* CurrentTarget = Unit->GetTargetActor();
	auto FIsTargetEligible = [this](const ASpUnit* Candidate)
	{
		return IsAggroTargetEligible(Candidate);
	};

	Unit->SetTargetActor(AggroComponent->GetBestTarget(CurrentTarget, 1.25f, FIsTargetEligible));
	SyncBlackboardTarget();
}
