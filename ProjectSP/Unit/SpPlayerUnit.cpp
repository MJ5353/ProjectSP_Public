#include "SpPlayerUnit.h"
#include "Component/Player/SpPlayerCameraComponent.h"
#include "Component/Player/SpPlayerInputComponent.h"
#include "Component/Player/SpPlayerActionComponent.h"
#include "ProjectSP/Ability/Core/SpAbilitySystemComponent.h"

// ==================================================

ASpPlayerUnit::ASpPlayerUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bFindCameraComponentWhenViewTarget = true;

	CameraComponent = CreateDefaultSubobject<USpPlayerCameraComponent>("SpPlayerCameraComponent");
	CameraComponent->SetupAttachment(GetRootComponent());
	CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	CameraComponent->SetAutoActivate(true);

	UnitInputComponent = CreateDefaultSubobject<USpPlayerInputComponent>("SpPlayerInputComponent");
	PlayerActionComponent = CreateDefaultSubobject<USpPlayerActionComponent>("SpPlayerActionComponent");
}

void ASpPlayerUnit::BeginPlay()
{
	if (HasAuthority())
		SpawnLocation = GetActorLocation();

	Super::BeginPlay();
}

void ASpPlayerUnit::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	TrySetInput(true);
}

void ASpPlayerUnit::OnRep_Controller()
{
	Super::OnRep_Controller();
	TrySetInput(true);
}

void ASpPlayerUnit::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	TrySetInput(true);
}

void ASpPlayerUnit::OnRep_UnitData()
{
	Super::OnRep_UnitData();
	TrySetInput(false);
}

void ASpPlayerUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	TrySetInput(false);
}

void ASpPlayerUnit::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority())
		SetUnitActive_Server(false, true);

	Super::EndPlay(EndPlayReason);
}

// protected

void ASpPlayerUnit::HandleDeadProcessFinished_Server()
{
	check(HasAuthority());

	// 충돌과 표시를 복구하기 전에 최초 스폰 위치로 돌아간다.
	SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
	
	if (AbilitySystemComponent)
		AbilitySystemComponent->RestoreHp();
	
	SetUnitPlayable(true);
	SetUnitPresentationVisible_Server(true);
}

void ASpPlayerUnit::TrySetInput(bool bInitUnit)
{
	if (!IsLocallyControlled() || !UnitInputComponent || !InputComponent)
		return;
	
	if (bInitUnit)
		InitUnit();
	
	UnitInputComponent->SetUp(InputComponent);
}
