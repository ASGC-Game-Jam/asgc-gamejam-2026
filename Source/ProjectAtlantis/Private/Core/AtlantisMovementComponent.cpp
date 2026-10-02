// Copyright (c) 2026 ASGC


#include "Core/AtlantisMovementComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "GameFramework/Character.h"

void UAtlantisMovementComponent::PhysSwimming(float DeltaTime, int32 Iterations)
{
	const AAtlantisPlayerState* PlayerState = CharacterOwner->GetPlayerState<AAtlantisPlayerState>();
	if (!PlayerState)
	{
		Super::PhysSwimming(DeltaTime, Iterations);
		return;
	}

	const float SavedBuoyancy = Buoyancy;
	const FVector SavedAcceleration = Acceleration;
	switch (PlayerState->GetBallastState())
	{
	case EAtlantisBallastState::Descend:
		Buoyancy = FMath::Clamp(DescendBuoyancy, 0.f, 0.99f);
		Acceleration = ProjectToGravityFloor(Acceleration);
		break;
	case EAtlantisBallastState::Ascend:
		Buoyancy = FMath::Max(AscendBuoyancy, 1.01f);
		Acceleration = ProjectToGravityFloor(Acceleration);
		break;
	case EAtlantisBallastState::Wander:
		Buoyancy = 1.f;
		break;
	default:
		break;
	}

	Super::PhysSwimming(DeltaTime, Iterations);
	Buoyancy = SavedBuoyancy;
	Acceleration = SavedAcceleration;
}

