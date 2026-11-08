// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PhysicsVolume.h"
#include "AtlantisWaterVolume.generated.h"

class UBoxComponent;

/** Brush-free water volume for bounded swimming spaces, including the ocean POC. */
UCLASS()
class PROJECTATLANTIS_API AAtlantisWaterVolume : public APhysicsVolume
{
	GENERATED_BODY()
public:
	AAtlantisWaterVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Water")
	TObjectPtr<UBoxComponent> WaterBoundary;
};
