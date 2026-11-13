// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AtlantisAICharacter.generated.h"

UCLASS()
class PROJECTATLANTIS_API AAtlantisAICharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Uses UAtlantisMovementComponent (same swimming as the player). Without a PlayerState it runs plain
	// engine swimming with no ballast, which gives AI neutral buoyancy (Buoyancy 1, GravityScale 0).
	AAtlantisAICharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
