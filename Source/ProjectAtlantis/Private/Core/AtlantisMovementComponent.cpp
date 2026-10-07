// Copyright (c) 2026 ASGC


#include "Core/AtlantisMovementComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Core/AtlantisPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"

UAtlantisMovementComponent::UAtlantisMovementComponent()
{
	MaxSwimAcceleration = 2000.f;
	BrakingDecelerationSwimming = 250.f;
	MaxVerticalSwimSpeed = 150.f;
}

float UAtlantisMovementComponent::GetMaxAcceleration() const
{
	return IsSwimming() ? MaxSwimAcceleration : Super::GetMaxAcceleration();
}

// Swimming follows the camera's pitch: W swims where you look, A/D to move to the sides, this will just add pitch
FVector UAtlantisMovementComponent::ConstrainInputAcceleration(const FVector& InputAcceleration) const
{
	// Keep stock behaviour for every other movement mode
	const FVector Constrained = Super::ConstrainInputAcceleration(InputAcceleration);

	if (!IsSwimming() || !CharacterOwner || !CharacterOwner->GetController())
	{
		return Constrained;
	}
	const FRotator ControlRotation = CharacterOwner->GetControlRotation();
	const FRotator YawRotator(0.f, ControlRotation.Yaw, 0.f);
	const FRotator AimRotator(ControlRotation.Pitch, ControlRotation.Yaw, 0.f);

	// We turn the camera-relative forward into where we are looking
	return AimRotator.RotateVector(YawRotator.UnrotateVector(Constrained));
}

void UAtlantisMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	const AAtlantisPlayerState* PlayerState = CharacterOwner
		                                          ? CharacterOwner->GetPlayerState<AAtlantisPlayerState>()
		                                          : nullptr;

	if (!PlayerState)
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	const bool bPassiveBallast = IsSwimming() && (PlayerState->GetBallastState() ==
		EAtlantisBallastState::Descend || PlayerState->GetBallastState() == EAtlantisBallastState::Ascend);
	
	// Save before clearing Z: passive buoyancy must accumulate across swimming steps.
	const FVector::FReal PassiveVerticalVelocity = GetGravitySpaceZ(Velocity);
	if (bPassiveBallast)
	{
		SetGravitySpaceZ(Velocity, 0.f);
	}

	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);

	if (bPassiveBallast)
	{
		const float FluidDrag = bFluid ? 1.f - FMath::Min(Friction * DeltaTime, 1.f) : 1.f;
		SetGravitySpaceZ(Velocity, PassiveVerticalVelocity * FluidDrag);
	}
	
	// We want to check the direction of our acceleration so we only apply it to the player when going up or down
	if (!IsSwimming() || FMath::IsNearlyZero(Acceleration.Z))
	{
		return;
	}
	
	const float HorizontalMaxSpeed = GetMaxSpeed();
	const float Speed = Velocity.Size();
	if (HorizontalMaxSpeed <= 0.f || MaxVerticalSwimSpeed <= 0.f || Speed <= KINDA_SMALL_NUMBER)
	{
		return;
	}
	
	const FVector Direction = Velocity / Speed;
	const float HorizontalRatio = Direction.Size2D() / HorizontalMaxSpeed;
	const float VerticalRatio = FMath::Abs(Direction.Z) / MaxVerticalSwimSpeed;
	const float MaxSpeedInDirection = 1.f / FMath::Sqrt(FMath::Square(HorizontalRatio) + FMath::Square(VerticalRatio));
	
	if (Speed > MaxSpeedInDirection)
	{
		Velocity = Direction * MaxSpeedInDirection;
	}
}

void UAtlantisMovementComponent::PhysicsVolumeChanged(APhysicsVolume* NewVolume)
{
	Super::PhysicsVolumeChanged(NewVolume);
	
	if (GetPhysicsVolume() == NewVolume || !CharacterOwner)
	{
		return;
	}
	
	if (AAtlantisPlayerController* Controller = Cast<AAtlantisPlayerController>(CharacterOwner->GetController()))
	{
		Controller->UpdateMovementControls(NewVolume && NewVolume->bWaterVolume);
	}
}

float UAtlantisMovementComponent::ImmersionDepth() const
{
	const AAtlantisPlayerState* PlayerState = CharacterOwner->GetPlayerState<AAtlantisPlayerState>();
	if (IsSwimming() && GetPhysicsVolume()->bWaterVolume && PlayerState)
	{
		const EAtlantisBallastState BallastState = PlayerState->GetBallastState();
		if (BallastState != EAtlantisBallastState::None)
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

	// Preserve configured buoyancy for native swimming when PlayerState is unavailable.
	const float SavedBuoyancy = Buoyancy;
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
}
