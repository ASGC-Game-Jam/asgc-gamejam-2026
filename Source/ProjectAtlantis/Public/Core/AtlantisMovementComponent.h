// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AtlantisMovementComponent.generated.h"

/**
 * Player movement component, so we can set up the stock Character Movement Component with our swim-specific tuning values
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UAtlantisMovementComponent();

	virtual void PhysicsRotation(float DeltaTime) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysicsVolumeChanged(APhysicsVolume* NewVolume) override;
	virtual float ImmersionDepth() const override;
	virtual float GetMaxAcceleration() const override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

protected:
	/** How quickly swimming facing converges to velocity or returns upright. Zero turns immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Swimming", meta = (ClampMin = "0"))
	float SwimFacingInterpolationSpeed = 8.f;

	/** At or below this speed, return upright with the last yaw rather than rotating to noise. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Swimming", meta = (ClampMin = "0"))
	float MinSwimFacingSpeed = 1.f;

	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const override;
	
	virtual void PhysSwimming(float DeltaTime, int32 Iterations) override;

	/** Ballast buoyancy ratio: below 1 sinks; 0 has no lift. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Ballast", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float DescendBuoyancy = 0.5f;

	/** Ballast buoyancy ratio: above 1 rises. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Ballast", meta = (ClampMin = "1.01"))
	float AscendBuoyancy = 1.5f;

private:
	/** 
	 * Acceleration while swimming. Separate from walking's Max Acceleration so swim feel can be tuned independently 
	 * See also Braking Deceleration Swimming
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement: Swimming", meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ForceUnits = "cm/s^2"))
	float MaxSwimAcceleration;
	
	/** 
	* Determines  the Maximum Vertical Swim Speed for the player
	* If you want to set the horizontal swim speed see Max Swim Speed
	*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement: Swimming", meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ForceUnits = "cm/s"))
	float MaxVerticalSwimSpeed;
};
