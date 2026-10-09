// Copyright (c) 2026 ASGC


#include "Core/AtlantisMovementComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Core/AtlantisPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/PhysicsVolume.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/ScopedMovementUpdate.h"

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
	if (IsSurfaceWalking() && CharacterOwner && CharacterOwner->GetController())
	{
		const FRotator Yaw(0.f, CharacterOwner->GetControlRotation().Yaw, 0.f);
		FVector Forward = FVector::VectorPlaneProject(CharacterOwner->GetControlRotation().Vector(), SurfaceNormal).GetSafeNormal();
		if (Forward.IsNearlyZero())
		{
			// Try camera yaw without pitch.
			Forward = FVector::VectorPlaneProject(Yaw.Vector(), SurfaceNormal).GetSafeNormal();
		}
		if (Forward.IsNearlyZero())
		{
			// Yaw can also point into a vertical surface, so use the character's facing.
			Forward = FVector::VectorPlaneProject(CharacterOwner->GetActorForwardVector(), SurfaceNormal).GetSafeNormal();
		}
		// Keep camera-right input pointing right even on an inverted ceiling.
		FVector Right = FVector::CrossProduct(SurfaceNormal, Forward).GetSafeNormal();
		if (FVector::DotProduct(Right, FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y)) < 0.f)
		{
			Right *= -1.f;
		}
		const FVector LocalInput = Yaw.UnrotateVector(InputAcceleration);
		return Forward * LocalInput.X + Right * LocalInput.Y;
	}
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
	// Native volume entry forces Swimming even for replicated custom movement.
	// Preserve Surface Walk within water; leaving water still uses native handling.
	if (!(IsSurfaceWalking() && NewVolume && NewVolume->bWaterVolume))
	{
		Super::PhysicsVolumeChanged(NewVolume);
	}
	
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
	if (TryAttachSurface())
	{
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}
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
		break;
	case EAtlantisBallastState::Ascend:
		Buoyancy = FMath::Max(AscendBuoyancy, 1.01f);
		break;
	case EAtlantisBallastState::Wander:
		Buoyancy = 1.f;
		break;
	default:
		break;
	}

	if (!PlayerState->IsVerticalSwimmingAllowed())
	{
		Acceleration = ProjectToGravityFloor(Acceleration);
	}

	Super::PhysSwimming(DeltaTime, Iterations);
	Buoyancy = SavedBuoyancy;
}

void UAtlantisMovementComponent::PhysicsRotation(float DeltaTime)
{
	if (IsSurfaceWalking())
	{
		if (!OrientToSurface())
		{
			DetachSurface(true);
		}
		return;
	}
	if (!IsSwimming())
	{
		Super::PhysicsRotation(DeltaTime);
		return;
	}
	if (!HasValidData())
	{
		return;
	}
	
	// Passive ballast motion should not tilt the character. On input release, return
	// upright immediately: residual vertical drift must not tilt the body farther.
	const AAtlantisPlayerState* PlayerState = CharacterOwner->GetPlayerState<AAtlantisPlayerState>();
	const bool bPassiveBallast = PlayerState &&
	(PlayerState->GetBallastState() == EAtlantisBallastState::Ascend ||
		PlayerState->GetBallastState() == EAtlantisBallastState::Descend);
	
	const bool bSwimmingWithInput = !Acceleration.IsNearlyZero() &&
		Velocity.SizeSquared() > FMath::Square(MinSwimFacingSpeed);
	
	const FRotator TargetRotation = !bPassiveBallast && bSwimmingWithInput
		                                ? Velocity.Rotation()
		                                : FRotator(0.f, UpdatedComponent->GetComponentRotation().Yaw, 0.f);
	const FQuat Target = TargetRotation.Quaternion();
	
	// Interpolate neutral pitch/roll independently so quaternion shortest-path
	// interpolation cannot gradually change the heading while righting a tilted pose.
	const FQuat Facing = !bPassiveBallast && bSwimmingWithInput
		                     ? FMath::QInterpTo(UpdatedComponent->GetComponentQuat(), Target, DeltaTime,
		                                        SwimFacingInterpolationSpeed)
		                     : FMath::RInterpTo(UpdatedComponent->GetComponentRotation(), TargetRotation, DeltaTime,
		                                        SwimFacingInterpolationSpeed).Quaternion();
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, Facing, true, Hit);
}

void UAtlantisMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	bDetachedThisFrame = false;
	SurfaceReattachTimeRemaining = FMath::Max(0.f, SurfaceReattachTimeRemaining - DeltaTime);
	AAtlantisPlayerState* State = CharacterOwner ? CharacterOwner->GetPlayerState<AAtlantisPlayerState>() : nullptr;
	if (State != BoundBallastState.Get())
	{
		if (BoundBallastState.IsValid())
		{
			BoundBallastState->OnBallastChanged.RemoveDynamic(this, &UAtlantisMovementComponent::OnSurfaceBallastChanged);
		}
		BoundBallastState = State;
		if (State)
		{
			State->OnBallastChanged.AddDynamic(this, &UAtlantisMovementComponent::OnSurfaceBallastChanged);
		}
	}
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (IsSurfaceWalking() && CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		SurfaceNormal = CharacterOwner->GetActorUpVector();
	}
}

bool UAtlantisMovementComponent::IsSurfaceAllowed(const FVector& Normal) const
{
	const AAtlantisPlayerState* State = CharacterOwner ? CharacterOwner->GetPlayerState<AAtlantisPlayerState>() : nullptr;
	if (!State || (IsSurfaceWalking() && State->GetBallastState() != AttachedBallastState))
	{
		return false;
	}
	// Ballast selects the side, but does not make steep slopes or walls walkable.
	const float RequiredNormalZ = FMath::Max(MinSurfaceVerticalNormal, GetWalkableFloorZ());
	return (State->IsLowerSurfaceWalkingAllowed() && Normal.Z >= RequiredNormalZ)
		|| (State->IsUpperSurfaceWalkingAllowed() && Normal.Z <= -RequiredNormalZ);
}

bool UAtlantisMovementComponent::FindSupportingSurface(const FVector& Direction, float ExtraDistance, FHitResult& Hit) const
{
	if (!CharacterOwner || !UpdatedComponent || !GetWorld())
	{
		return false;
	}
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float Extent = Radius + (HalfHeight - Radius) * FMath::Abs(FVector::DotProduct(Direction, Capsule->GetUpVector()));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SurfaceWalkSupport), false, CharacterOwner);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	bool bHit;
	if (IsSwimming())
	{
		bHit = GetWorld()->SweepSingleByChannel(Hit, Start, Start + Direction * (ExtraDistance + 1.f),
			Capsule->GetComponentQuat(), UpdatedComponent->GetCollisionObjectType(),
			FCollisionShape::MakeCapsule(FMath::Max(Radius - 1.f, 1.f), FMath::Max(HalfHeight - 1.f, Radius)), Params);
	}
	else
	{
		bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Direction * (Extent + ExtraDistance),
			UpdatedComponent->GetCollisionObjectType(), Params);
	}
	return bHit && Hit.bBlockingHit && !Hit.bStartPenetrating && IsSurfaceAllowed(Hit.ImpactNormal);
}

bool UAtlantisMovementComponent::TryAttachSurface()
{
	if (bDetachedThisFrame || SurfaceReattachTimeRemaining > 0.f || !IsSwimming() || !GetPhysicsVolume() || !GetPhysicsVolume()->bWaterVolume || !CharacterOwner)
	{
		return false;
	}
	const AAtlantisPlayerState* State = CharacterOwner->GetPlayerState<AAtlantisPlayerState>();
	if (!State || (!State->IsLowerSurfaceWalkingAllowed() && !State->IsUpperSurfaceWalkingAllowed()))
	{
		return false;
	}
	FHitResult Hit;
	const FVector Direction = State->IsLowerSurfaceWalkingAllowed() ? -FVector::UpVector : FVector::UpVector;
	if (!FindSupportingSurface(Direction, SurfaceAttachDistance, Hit))
	{
		return false;
	}
	SurfaceNormal = Hit.ImpactNormal;
	const FVector Target = Hit.ImpactPoint + SurfaceNormal * (CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + SurfaceContactOffset);
	const FQuat Rotation = GetSurfaceRotation();
	if (IsSurfacePoseBlocked(Target, Rotation))
	{
		SurfaceReattachTimeRemaining = BlockedSurfaceReattachDelay;
		return false;
	}
	FScopedMovementUpdate ScopedAttachment(UpdatedComponent, EScopedUpdate::DeferredUpdates);
	FHitResult PositionHit;
	SafeMoveUpdatedComponent(Target - UpdatedComponent->GetComponentLocation(), Rotation, true, PositionHit);
	if (!UpdatedComponent->GetComponentLocation().Equals(Target, 1.f)
		|| IsSurfacePoseBlocked(UpdatedComponent->GetComponentLocation(), Rotation))
	{
		ScopedAttachment.RevertMove();
		SurfaceReattachTimeRemaining = BlockedSurfaceReattachDelay;
		return false;
	}
	AttachedBallastState = State->GetBallastState();
	Velocity = FVector::VectorPlaneProject(Velocity, SurfaceNormal);
	SetMovementMode(MOVE_Custom, State->IsUpperSurfaceWalkingAllowed() ? 2 : 1);
	SetBase(Hit.GetComponent(), Hit.BoneName);
	OrientToSurface();
	return true;
}

FQuat UAtlantisMovementComponent::GetSurfaceRotation() const
{
	FVector Forward = FVector::VectorPlaneProject(CharacterOwner->GetActorForwardVector(), SurfaceNormal).GetSafeNormal();
	if (!Velocity.IsNearlyZero())
	{
		Forward = FVector::VectorPlaneProject(Velocity, SurfaceNormal).GetSafeNormal();
	}
	if (Forward.IsNearlyZero())
	{
		// Facing or velocity along the normal has no tangent; choose a valid surface axis to build the rotation.
		FVector Right;
		SurfaceNormal.FindBestAxisVectors(Forward, Right);
	}
	return FRotationMatrix::MakeFromXZ(Forward, SurfaceNormal).ToQuat();
}

bool UAtlantisMovementComponent::IsSurfacePoseBlocked(const FVector& Location, const FQuat& Rotation) const
{
	const UCapsuleComponent* Capsule = CharacterOwner->GetCapsuleComponent();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SurfaceWalkPose), false, CharacterOwner);
	return GetWorld()->OverlapBlockingTestByChannel(Location, Rotation, Capsule->GetCollisionObjectType(),
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params);
}

bool UAtlantisMovementComponent::OrientToSurface()
{
	const FQuat Rotation = GetSurfaceRotation();
	// Native rotation is not swept: reject a tilted pose that would embed the capsule in a neighbour.
	if (IsSurfacePoseBlocked(UpdatedComponent->GetComponentLocation(), Rotation))
	{
		return false;
	}
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, Rotation, true, Hit);
	return true;
}

bool UAtlantisMovementComponent::TrySurfaceStep(const FVector& Delta)
{
	if (MaxStepHeight <= 0.f || Delta.IsNearlyZero())
	{
		return false;
	}
	FScopedMovementUpdate ScopedStep(UpdatedComponent, EScopedUpdate::DeferredUpdates);
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();
	FHitResult Hit;
	SafeMoveUpdatedComponent(SurfaceNormal * MaxStepHeight, Rotation, true, Hit);
	if (Hit.bBlockingHit)
	{
		ScopedStep.RevertMove();
		return false;
	}
	SafeMoveUpdatedComponent(Delta, Rotation, true, Hit);
	if (Hit.bBlockingHit)
	{
		ScopedStep.RevertMove();
		return false;
	}
	SafeMoveUpdatedComponent(-SurfaceNormal * (MaxStepHeight + SurfaceDetachDistance), Rotation, true, Hit);
	if (!Hit.IsValidBlockingHit() || !IsSurfaceAllowed(Hit.ImpactNormal)
		|| FVector::DotProduct(SurfaceNormal, Hit.ImpactNormal) < FMath::Cos(FMath::DegreesToRadians(MaxSurfaceNormalChangeDegrees))
		|| FVector::DotProduct(UpdatedComponent->GetComponentLocation() - Start, SurfaceNormal) > MaxStepHeight + KINDA_SMALL_NUMBER)
	{
		ScopedStep.RevertMove();
		return false;
	}
	// Keep a small clearance instead of leaving the stepped capsule touching the new support.
	SafeMoveUpdatedComponent(Hit.ImpactNormal * SurfaceContactOffset, Rotation, true, Hit);
	return true;
}

bool UAtlantisMovementComponent::SnapToSurface(const FHitResult& Support)
{
	FScopedMovementUpdate ScopedAlignment(UpdatedComponent, EScopedUpdate::DeferredUpdates);
	SurfaceNormal = Support.ImpactNormal;
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const float HalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Distance = FVector::DotProduct(Start - Support.ImpactPoint, SurfaceNormal);
	const FVector Delta = SurfaceNormal * (HalfHeight + SurfaceContactOffset - Distance);
	FHitResult Hit;
	// Restore clearance before rotating onto a different normal, avoiding penetration of the support itself.
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (!UpdatedComponent->GetComponentLocation().Equals(Start + Delta, 1.f) || !OrientToSurface())
	{
		ScopedAlignment.RevertMove();
		return false;
	}
	return true;
}

void UAtlantisMovementComponent::DetachSurface(bool bBlocked)
{
	if (!IsSurfaceWalking())
	{
		return;
	}
	bDetachedThisFrame = true;
	if (bBlocked)
	{
		SurfaceReattachTimeRemaining = BlockedSurfaceReattachDelay;
	}
	SetBase(nullptr);
	SurfaceNormal = FVector::UpVector;
	FHitResult Hit;
	SafeMoveUpdatedComponent(FVector::ZeroVector, FRotator(0.f, CharacterOwner->GetActorRotation().Yaw, 0.f).Quaternion(), true, Hit);
	SetMovementMode(GetPhysicsVolume() && GetPhysicsVolume()->bWaterVolume ? MOVE_Swimming : MOVE_Falling);
}

void UAtlantisMovementComponent::OnSurfaceBallastChanged(bool bAllocated, EAtlantisBallastState State)
{
	if (CharacterOwner && CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy && IsSurfaceWalking() && !IsSurfaceAllowed(SurfaceNormal))
	{
		DetachSurface();
	}
}

float UAtlantisMovementComponent::GetMaxSpeed() const
{
	return IsSurfaceWalking() ? MaxWalkSpeed : Super::GetMaxSpeed();
}

void UAtlantisMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (!IsSurfaceWalking())
	{
		Super::PhysCustom(DeltaTime, Iterations);
		return;
	}
	// Remote proxies follow replicated attachment and rotation; they do not decide support loss.
	if (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(Velocity * DeltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
		return;
	}
	FHitResult Support;
	if (!GetPhysicsVolume() || !GetPhysicsVolume()->bWaterVolume
		|| !FindSupportingSurface(-SurfaceNormal, SurfaceDetachDistance, Support)
		|| FVector::DotProduct(SurfaceNormal, Support.ImpactNormal) < FMath::Cos(FMath::DegreesToRadians(MaxSurfaceNormalChangeDegrees)))
	{
		DetachSurface();
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}
	if (!SnapToSurface(Support))
	{
		DetachSurface(true);
		StartNewPhysics(DeltaTime, Iterations);
		return;
	}
	SetBase(Support.GetComponent(), Support.BoneName);
	Acceleration = FVector::VectorPlaneProject(Acceleration, SurfaceNormal);
	Velocity = FVector::VectorPlaneProject(Velocity, SurfaceNormal);
	CalcVelocity(DeltaTime, GroundFriction, false, BrakingDecelerationWalking);
	Velocity = FVector::VectorPlaneProject(Velocity, SurfaceNormal).GetClampedToMaxSize(GetMaxSpeed());
	const FVector Start = UpdatedComponent->GetComponentLocation();
	FHitResult Hit;
	SafeMoveUpdatedComponent(Velocity * DeltaTime, UpdatedComponent->GetComponentQuat(), true, Hit);
	if (Hit.IsValidBlockingHit())
	{
		const FVector RemainingDelta = Velocity * DeltaTime * (1.f - Hit.Time);
		if (!TrySurfaceStep(RemainingDelta))
		{
			SlideAlongSurface(Velocity * DeltaTime, 1.f - Hit.Time, Hit.Normal, Hit, true);
			if (!Acceleration.IsNearlyZero() && FVector::VectorPlaneProject(UpdatedComponent->GetComponentLocation() - Start, SurfaceNormal).SizeSquared() < 0.01f)
			{
				DetachSurface(true);
				return;
			}
		}
	}
	if (!FindSupportingSurface(-SurfaceNormal, SurfaceDetachDistance, Support)
		|| FVector::DotProduct(SurfaceNormal, Support.ImpactNormal) < FMath::Cos(FMath::DegreesToRadians(MaxSurfaceNormalChangeDegrees)))
	{
		DetachSurface();
		return;
	}
	if (!SnapToSurface(Support))
	{
		DetachSurface(true);
		return;
	}
	SetBase(Support.GetComponent(), Support.BoneName);
	if (DeltaTime > MIN_TICK_TIME)
	{
		Velocity = FVector::VectorPlaneProject((UpdatedComponent->GetComponentLocation() - Start) / DeltaTime, SurfaceNormal);
	}
}

void UAtlantisMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);
	// Surface Walk owns its orientation; only restore upright rotation on terrestrial exits.
	if (PreviousMovementMode == MOVE_Swimming && !IsSwimming() && !IsSurfaceWalking() && HasValidData())
	{
		FHitResult Hit;
		SafeMoveUpdatedComponent(FVector::ZeroVector,
		                         FRotator(0.f, UpdatedComponent->GetComponentRotation().Yaw, 0.f).Quaternion(), true,
		                         Hit);
	}
	if (IsSurfaceWalking() && AttachedBallastState == EAtlantisBallastState::None)
	{
		// Network movement mode can arrive before local contact detection.
		AttachedBallastState = CustomMovementMode == 2 ? EAtlantisBallastState::Ascend : EAtlantisBallastState::Descend;
		SurfaceNormal = CharacterOwner->GetActorUpVector();
	}
	else if (!IsSurfaceWalking())
	{
		AttachedBallastState = EAtlantisBallastState::None;
	}
}
