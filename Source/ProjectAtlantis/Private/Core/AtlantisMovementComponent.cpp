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
	
	//We return early if we need to
	if (!IsSwimming() || FMath::IsNearlyZero(Acceleration.Z))
	{
		return;
	}
	
	//We select the Limiting mode
	//Also, I'm leaving no default option just in case of failure so we can have a warning
	switch (VerticalLimitMode)
	{
	case EAtlantisSwimVerticalLimit::Clamp:
		Velocity = ClampVerticalSpeed(Velocity);
		break;
	case EAtlantisSwimVerticalLimit::Scale:
		Velocity = ScaleToVerticalSpeed(Velocity);
		break;
		
	case EAtlantisSwimVerticalLimit::Ellipse:
		Velocity = LimitToSpeedEllipse(Velocity);
		break;
		
		
	}
}

FVector UAtlantisMovementComponent::ClampVerticalSpeed(const FVector& InVelocity) const
{
	// We only want to limit the vertical part here, bc the hotizontal speed is already handled by Max Swimming Speed
	FVector OutVelocity = InVelocity;
	OutVelocity.Z = FMath::Clamp(InVelocity.Z, -MaxVerticalSwimSpeed, MaxVerticalSwimSpeed);
	return OutVelocity;
}

// We will want to shrink the whole velocity until the vertical part fits
FVector UAtlantisMovementComponent::ScaleToVerticalSpeed(const FVector& InVelocity) const
{
	const float VerticalSpeed = FMath::Abs(InVelocity.Z);
	
	if (VerticalSpeed <= MaxVerticalSwimSpeed)
	{
		return InVelocity;
	}
	
	return InVelocity * (MaxVerticalSwimSpeed/VerticalSpeed);
}

FVector UAtlantisMovementComponent::LimitToSpeedEllipse(const FVector& InVelocity) const
{
	const float HorizontalMaxSpeed = GetMaxSpeed();
	const float Speed = InVelocity.Size();
	
	if (HorizontalMaxSpeed <= 0.f || MaxVerticalSwimSpeed <= 0.f || Speed <= KINDA_SMALL_NUMBER) 
	{
		return InVelocity;
	}
	
	const FVector Direction = InVelocity / Speed;
	const float HorizontalRatio = Direction.Size2D() / HorizontalMaxSpeed;
	const float VerticalRatio = FMath::Abs(Direction.Z) / MaxVerticalSwimSpeed;
	
	const float MaxSpeedInDirection = 1.f / FMath::Sqrt(FMath::Square(HorizontalRatio) + FMath::Square(VerticalRatio));
	
	return Speed > MaxSpeedInDirection ? Direction * MaxSpeedInDirection : InVelocity;
}

