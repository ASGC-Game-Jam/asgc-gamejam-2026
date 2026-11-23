// Copyright (c) 2026 ASGC


#include "Core/AtlantisAICharacter.h"

#include "Core/AtlantisMovementComponent.h"


// Sets default values
AAtlantisAICharacter::AAtlantisAICharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UAtlantisMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AAtlantisAICharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAtlantisAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}