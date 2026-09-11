// Copyright (c) 2026 ASGC


#include "Core/AtlantisPlayerCharacter.h"

// Sets default values
AAtlantisPlayerCharacter::AAtlantisPlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AAtlantisPlayerCharacter::BeginPlay()
{
	StartTransform = GetActorTransform();
	Super::BeginPlay();
}

// Called every frame
void AAtlantisPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
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


