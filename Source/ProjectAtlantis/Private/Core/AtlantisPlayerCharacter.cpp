// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerCharacter.h"
#include "Core/AtlantisMovementComponent.h"
#include "Core/AtlantisPlayerState.h"

AAtlantisPlayerCharacter::AAtlantisPlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer.SetDefaultSubobjectClass<UAtlantisMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AAtlantisPlayerCharacter::BeginPlay()
{
	StartTransform = GetActorTransform();
	Super::BeginPlay();
}

void AAtlantisPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AAtlantisPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AAtlantisPlayerCharacter::MoveToStart()
{
	SetActorTransform(StartTransform);
}

void AAtlantisPlayerCharacter::MoveToTransform(const FTransform& Transform)
{
	SetActorTransform(Transform);
}

void AAtlantisPlayerCharacter::UpdateTraversalMode() const
{
	if (AAtlantisPlayerState* State  = GetPlayerState<AAtlantisPlayerState>())
	{
		State ->SetTraversalMode(ToTraversalMode(GetCharacterMovement()->MovementMode));
	}
}

EAtlantisTraversalMode AAtlantisPlayerCharacter::ToTraversalMode(const EMovementMode MovementMode)
{
	switch (MovementMode)
	{
	case MOVE_Walking:
	case MOVE_NavWalking:
	case MOVE_Falling:
		return EAtlantisTraversalMode::Terrestrial;
	case MOVE_Swimming:
		return EAtlantisTraversalMode::Swimming;
	default:
		return EAtlantisTraversalMode::None;
	}
}

void AAtlantisPlayerCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	UpdateTraversalMode();
}

void AAtlantisPlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	UpdateTraversalMode();
}

void AAtlantisPlayerCharacter::UnPossessed()
{
	if (AAtlantisPlayerState* State  = GetPlayerState<AAtlantisPlayerState>())
	{
		State ->SetTraversalMode(EAtlantisTraversalMode::None);
	}

	Super::UnPossessed();
}
