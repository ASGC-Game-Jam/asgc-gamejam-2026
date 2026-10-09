// Copyright (c) 2026 ASGC

#include "Core/AtlantisPlayerAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UAtlantisPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	bIsSwimming = Movement && Movement->IsSwimming();
	bIsSwimmingMoving = bIsSwimming
		&& !Movement->GetCurrentAcceleration().IsNearlyZero()
		&& Movement->Velocity.SizeSquared() > FMath::Square(SwimmingMovementThreshold);
}
