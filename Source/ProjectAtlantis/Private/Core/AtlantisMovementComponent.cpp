// Copyright (c) 2026 ASGC


#include "Core/AtlantisMovementComponent.h"
#include "GameFramework/Character.h"

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
	Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
	
	//We want to check the direction of our acceleration so we only apply it to the player when he goes up or down
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


