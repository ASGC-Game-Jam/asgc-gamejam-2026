// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PhysicsVolume.h"
#include "AtlantisAirPocketVolume.generated.h"

class USphereComponent;

/** A spherical dry physics volume inside water. Uses native volume priority for traversal. */
UCLASS()
class PROJECTATLANTIS_API AAtlantisAirPocketVolume : public APhysicsVolume
{
	GENERATED_BODY()

public:
	AAtlantisAirPocketVolume();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Air Pocket")
	TObjectPtr<USphereComponent> AirBoundary;

	/** Oxygen units restored per second while this is the character's active physics volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Air Pocket", meta = (ClampMin = "0.0"))
	float OxygenRestoreRate = 20.f;
};
