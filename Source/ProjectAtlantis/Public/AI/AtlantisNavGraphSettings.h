// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AtlantisNavGraphSettings.generated.h"

/** Project-wide settings for the swimming AI nav graph (Project Settings > Game > Atlantis AI Navigation). */
UCLASS(config = Game, DefaultConfig, meta = (DisplayName = "Atlantis AI Navigation"))
class PROJECTATLANTIS_API UAtlantisNavGraphSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	virtual FName GetCategoryName() const override { return TEXT("Game"); }

	/** Nodes further apart than this are never linked. */
	UPROPERTY(config, EditAnywhere, Category = "Graph", meta = (ClampMin = 100, UIMin = 100, UIMax = 5000, ForceUnits = "cm"))
	float MaxLinkDistance = 1200.f;

	/** Radius of the capsule swept between nodes to check an enemy fits through. */
	UPROPERTY(config, EditAnywhere, Category = "Agent", meta = (ClampMin = 1, UIMin = 1, UIMax = 200, ForceUnits = "cm"))
	float AgentRadius = 40.f;

	/** Half height of the swept capsule (enemies swim upright). */
	UPROPERTY(config, EditAnywhere, Category = "Agent", meta = (ClampMin = 1, UIMin = 1, UIMax = 300, ForceUnits = "cm"))
	float AgentHalfHeight = 90.f;

	/** How many of the nearest nodes are tested for a clear line when entering or leaving the graph. */
	UPROPERTY(config, EditAnywhere, Category = "Queries", meta = (ClampMin = 1, UIMin = 1, UIMax = 32))
	int32 MaxNearestCandidates = 8;
};
