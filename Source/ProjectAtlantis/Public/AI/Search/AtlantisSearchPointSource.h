// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisSearchPointSource.generated.h"

/** What a search point source gets to work with. */
struct FAtlantisSearchPointQuery
{
	UWorld* World = nullptr;
	/** The searcher's current location (pawn centre). Points must be reachable from here. */
	FVector From = FVector::ZeroVector;
	/** Where the search started (usually the player's last known location). */
	FVector Origin = FVector::ZeroVector;
	/** Points already checked during this search. */
	TConstArrayView<FVector> Visited;
};

/**
 * Picks the next place to check during a search (seam S7). Subclass for other strategies (hiding spots, auto-grid...).
 * Used through an instanced struct on the Find Search Point task.
 */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FAtlantisSearchPointSource
{
	GENERATED_BODY()

	virtual ~FAtlantisSearchPointSource() = default;

	/** Writes a point to check into OutPoint. False when there's nothing (left) to check. */
	virtual bool FindPoint(const FAtlantisSearchPointQuery& Query, FVector& OutPoint) const { return false; }
};

/** Random reachable swim nav graph node near the search origin that hasn't been checked yet. */
USTRUCT(BlueprintType, meta = (DisplayName = "Swim Nav Graph Nodes"))
struct PROJECTATLANTIS_API FAtlantisSearchPointSource_NavGraph : public FAtlantisSearchPointSource
{
	GENERATED_BODY()

	/** Only nodes this close to the search origin are candidates. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|AI|Search", meta = (ClampMin = 0, UIMin = 0, ForceUnits = "cm"))
	float Radius = 1500.f;

	/** Skip nodes this close to the origin (the searcher already looked there). */
	UPROPERTY(EditAnywhere, Category = "Atlantis|AI|Search", meta = (ClampMin = 0, UIMin = 0, ForceUnits = "cm"))
	float MinDistanceFromOrigin = 200.f;

	/** A node within this distance of an already checked point counts as checked. */
	UPROPERTY(EditAnywhere, Category = "Atlantis|AI|Search", meta = (ClampMin = 0, UIMin = 0, ForceUnits = "cm"))
	float VisitedTolerance = 150.f;

	virtual bool FindPoint(const FAtlantisSearchPointQuery& Query, FVector& OutPoint) const override;
};
