#include "SpPlayerUnit.h"
#include "Component/Player/SpUnitCameraComponent.h"
#include "Component/Player/SpUnitInputComponent.h"
#include "Component/Player/SpPlayerCommandComponent.h"
#include "ProjectSP/Ability/SpAbilitySystemComponent.h"
#include "ProjectSP/GameFramework/SpPlayerState.h"

// ==================================================

ASpPlayerUnit::ASpPlayerUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bFindCameraComponentWhenViewTarget = true;

	CameraComponent = CreateDefaultSubobject<USpUnitCameraComponent>("SpUnitCameraComponent");
	CameraComponent->SetupAttachment(GetRootComponent());
	CameraComponent->SetRelativeLocation(FVector(-300.0f, 0.0f, 75.0f));
	CameraComponent->SetAutoActivate(true);

	UnitInputComponent = CreateDefaultSubobject<USpUnitInputComponent>("SpUnitInputComponent");
	PlayerCommandComponent = CreateDefaultSubobject<USpPlayerCommandComponent>("SpPlayerCommandComponent");
}

void ASpPlayerUnit::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
	TrySetInput();
}

void ASpPlayerUnit::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilitySystem();
	TrySetInput();
}

void ASpPlayerUnit::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
	TrySetInput();
}

void ASpPlayerUnit::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	TrySetInput();
}

UAbilitySystemComponent* ASpPlayerUnit::GetAbilitySystemComponent() const
{
	return GetSpAbilitySystemComponent();
}

USpAbilitySystemComponent* ASpPlayerUnit::GetSpAbilitySystemComponent() const
{
	if (const ASpPlayerState* SpPlayerState = GetPlayerState<ASpPlayerState>())
		return SpPlayerState->GetSpAbilitySystemComponent();

	return nullptr;
}

void ASpPlayerUnit::InitializeAbilitySystem()
{
	if (ASpPlayerState* SpPlayerState = GetPlayerState<ASpPlayerState>())
	{
		// PlayerState가 Owner, 이 Pawn이 Avatar. 유닛 교체 시에도 플레이어 GAS 상태를 유지한다.
		SpPlayerState->InitializeAbilitySystem(this);
		RegisterUnitTagChangedEvent();
	}
}

void ASpPlayerUnit::TrySetInput()
{
	if (!IsLocallyControlled() || !UnitInputComponent || !InputComponent)
		return;

	UnitInputComponent->SetUp(InputComponent);
}
