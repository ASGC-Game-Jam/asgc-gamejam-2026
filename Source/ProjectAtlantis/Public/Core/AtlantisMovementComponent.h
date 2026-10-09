// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Core/AtlantisPlayerState.h"
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

	/** Surface Walk uses a native custom movement mode; normal swimming resumes on detachment. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Surface Walk")
	bool IsSurfaceWalking() const { return MovementMode == MOVE_Custom && (CustomMovementMode == 1 || CustomMovementMode == 2); }

	UFUNCTION(BlueprintPure, Category = "Atlantis|Surface Walk")
	FVector GetSupportingSurfaceNormal() const { return SurfaceNormal; }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void PhysicsRotation(float DeltaTime) override;
	virtual float GetMaxSpeed() const override;
	virtual void PhysicsVolumeChanged(APhysicsVolume* NewVolume) override;
	virtual float ImmersionDepth() const override;
	virtual float GetMaxAcceleration() const override;
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;

protected:
	/** How quickly active swimming faces velocity or returns upright after input release. Zero turns immediately. */
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
	/** Distance beyond the capsule at which automatic attachment is allowed, in cm. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|Surface Walk", meta = (ClampMin = "0"))
	float SurfaceAttachDistance = 8.f;

	/** Support can remain this far beyond the capsule before detachment, in cm. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|Surface Walk", meta = (ClampMin = "0"))
	float SurfaceDetachDistance = 20.f;

	UPROPERTY(EditAnywhere, Category = "Atlantis|Surface Walk", meta = (ClampMin = "0"))
	float SurfaceContactOffset = 2.f;

	/** Largest normal change accepted in one step; larger corners detach. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|Surface Walk", meta = (ClampMin = "0", ClampMax = "89"))
	float MaxSurfaceNormalChangeDegrees = 60.f;

	/** Additional normal threshold, combined with Walkable Floor Angle for all support checks. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|Surface Walk", meta = (ClampMin = "0.01", ClampMax = "1"))
	float MinSurfaceVerticalNormal = 0.1f;

	FVector SurfaceNormal = FVector::UpVector;
	EAtlantisBallastState AttachedBallastState = EAtlantisBallastState::None;
	TWeakObjectPtr<AAtlantisPlayerState> BoundBallastState;
	bool bDetachedThisFrame = false;
	/** Delay retries after an obstructed pose or movement so Swimming can clear the geometry. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|Surface Walk", meta = (ClampMin = "0"))
	float BlockedSurfaceReattachDelay = 0.2f;
	float SurfaceReattachTimeRemaining = 0.f;
	bool FindSupportingSurface(const FVector& Direction, float ExtraDistance, FHitResult& Hit) const;
	bool IsSurfaceAllowed(const FVector& Normal) const;
	bool TryAttachSurface();
	void DetachSurface(bool bBlocked = false);
	bool OrientToSurface();
	FQuat GetSurfaceRotation() const;
	bool IsSurfacePoseBlocked(const FVector& Location, const FQuat& Rotation) const;
	bool TrySurfaceStep(const FVector& Delta);
	bool SnapToSurface(const FHitResult& Support);

	UFUNCTION()
	void OnSurfaceBallastChanged(bool bAllocated, EAtlantisBallastState State);

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
