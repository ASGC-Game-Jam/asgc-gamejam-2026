// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AtlantisNavNodeSource.generated.h"

/** A node offered to the swimming nav graph. */
struct FAtlantisNavNodeDesc
{
	/** Pawn-centre location (where the agent's capsule centre should be). */
	FVector Location = FVector::ZeroVector;
	/** Allowed traversal modes, see Atlantis::AI::TraversalModeToMask. */
	uint8 ModeMask = 0;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UAtlantisNavNodeSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything that contributes nodes to the swimming nav graph. Placed AAtlantisNavNode actors are the first source;
 * an auto-grid volume can be added later as another source without touching pathfinding.
 */
class PROJECTATLANTIS_API IAtlantisNavNodeSource
{
	GENERATED_BODY()

public:
	virtual void GatherNavNodes(TArray<FAtlantisNavNodeDesc>& OutNodes) const PURE_VIRTUAL(IAtlantisNavNodeSource::GatherNavNodes, );
};
