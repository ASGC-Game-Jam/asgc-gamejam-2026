// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AtlantisPlayerAnimInstance.generated.h"

/** Movement state for the player's land and underwater animation graph. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Atlantis|Animation")
	bool bIsSwimming = false;

	/** Passive ballast drift uses the idle loop; active movement uses the forward loop. */
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Atlantis|Animation")
	bool bIsSwimmingMoving = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Animation", meta = (ClampMin = "0", Units = "cm/s"))
	float SwimmingMovementThreshold = 3.0f;
};
