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
	if (!FMath::IsNearlyZero(Acceleration.Z))
	{
		//We clamp it between the up or down speeds (negative would be down...I think)
		Velocity.Z = FMath::Clamp(Velocity.Z, -MaxVerticalSwimSpeed, MaxVerticalSwimSpeed);
	}
}


