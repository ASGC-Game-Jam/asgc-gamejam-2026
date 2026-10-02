// Copyright (c) 2026 ASGC


#include "Core/AtlantisMovementComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"

void UAtlantisMovementComponent::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
	const APhysicsVolume* OldVolume = GetPhysicsVolume();
	if (OldVolume == NewVolume)
	{
		Super::PhysicsVolumeChanged(NewVolume);
		return;
	}
	
	if (OldVolume)
	{
		UE_LOG(LogTemp, Log, TEXT("%s exited PhysicsVolume %s (WaterVolume=%s)"),
			*GetNameSafe(GetOwner()), *GetNameSafe(OldVolume),
			OldVolume->bWaterVolume ? TEXT("true") : TEXT("false"));
	}
	
	if (NewVolume)
	{
		UE_LOG(LogTemp, Log, TEXT("%s entered PhysicsVolume %s (WaterVolume=%s)"),
			*GetNameSafe(GetOwner()), *GetNameSafe(NewVolume),
			NewVolume->bWaterVolume ? TEXT("true") : TEXT("false"));
		
		if (NewVolume->bWaterVolume)
		{
			AAtlantisPlayerState* PlayerState = CharacterOwner->GetPlayerState<AAtlantisPlayerState>();
			PlayerState->SetBallastState(EAtlantisBallastState::Descend);
		}
	}

	Super::PhysicsVolumeChanged(NewVolume);
}

float UAtlantisMovementComponent::ImmersionDepth() const
{
	if (const AAtlantisPlayerState* PlayerState = CharacterOwner->GetPlayerState<AAtlantisPlayerState>();
		IsSwimming() && GetPhysicsVolume()->bWaterVolume && PlayerState)
	{
		if (const EAtlantisBallastState BallastState = PlayerState->GetBallastState(); BallastState ==
			EAtlantisBallastState::Descend
			|| BallastState == EAtlantisBallastState::Wander
			|| BallastState == EAtlantisBallastState::Ascend)
		{
			// Ballast must retain its direction and neutral state throughout a water volume.
			// Native surface-depth scaling otherwise turns neutral buoyancy into sinking,
			// and can make Ascend sink as the capsule approaches the volume boundary.
			return 1.f;
		}
	}

	return Super::ImmersionDepth();
}

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
		if (FMath::IsNearlyZero(GetGravitySpaceZ(Acceleration))
			&& !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
		{
			// Neutral buoyancy cancels gravity, but does not remove existing sink/rise momentum.
			// Hold depth when there is no vertical input, even while swimming horizontally.
			SetGravitySpaceZ(Velocity, 0.f);
		}
		break;
	default:
		break;
	}

	Super::PhysSwimming(DeltaTime, Iterations);
	Buoyancy = SavedBuoyancy;
	Acceleration = SavedAcceleration;
}

