// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AtlantisMovementComponent.generated.h"

/** Applies Ballast buoyancy through Unreal's native swimming physics. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

protected:
	virtual void PhysSwimming(float DeltaTime, int32 Iterations) override;

	/** Ballast buoyancy ratio: below 1 sinks; 0 has no lift. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Ballast", meta = (ClampMin = "0.0", ClampMax = "0.99"))
	float DescendBuoyancy = 0.5f;

	/** Ballast buoyancy ratio: above 1 rises. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Ballast", meta = (ClampMin = "1.01"))
	float AscendBuoyancy = 1.5f;

};
