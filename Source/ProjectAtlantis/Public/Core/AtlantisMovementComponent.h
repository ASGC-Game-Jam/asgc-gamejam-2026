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
	
	virtual float GetMaxAcceleration() const override;

protected:
	virtual FVector ConstrainInputAcceleration(const FVector& InputAcceleration) const override;

private:
	/** 
	 * Acceleration while swimming. Separate from walking's Max Acceleration so swim feel can be tuned independently 
	 * See also Braking Deceleration swimming
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Character Movement: Swimming", meta = (AllowPrivateAccess = true, ClampMin = 0, UIMin = 0, ForceUnits = "cm/s^2"))
	float MaxSwimAcceleration;
};

