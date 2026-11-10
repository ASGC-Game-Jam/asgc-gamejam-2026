// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/AtlantisNavGraph.h"
#include "AtlantisNavGraphSubsystem.generated.h"

UENUM(BlueprintType)
enum class EAtlantisNavPathResult : uint8
{
	/** Straight, clear line from start to goal; the graph wasn't needed. */
	Direct,
	/** Path found through the graph. */
	Graph,
	/** No graph node is visible from the start. */
	NoStartNode,
	/** No graph node is visible from the goal. */
	NoEndNode,
	/** Start and goal nodes aren't connected. */
	Unreachable
};

/**
 * Navigator for swimming enemies: owns the 3D waypoint graph of the world and answers path queries.
 * The graph is static: built once at BeginPlay from every IAtlantisNavNodeSource in the world
 * (lazily on first query in editor worlds). Call Rebuild() after moving nodes at runtime.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisNavGraphSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	//~ FTickableGameObject (debug drawing only)
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Rebuilds the graph from all node sources in the world. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Navigation")
	void Rebuild();

	/** True if an enemy-sized upright capsule can move in a straight line between the points (pawns ignored). */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Navigation")
	bool IsSegmentClear(const FVector& From, const FVector& To) const;

	/**
	 * Finds a 3D path between two pawn-centre locations. Uses a direct leg when the straight line is clear
	 * (and bAllowDirectLeg), otherwise enters the graph at the nearest visible node and leaves it at the node
	 * nearest the goal.
	 */
	EAtlantisNavPathResult FindPath(const FVector& From, const FVector& To, const FAtlantisNavQueryFilter& Filter, bool bAllowDirectLeg, FAtlantisNavPath& OutPath);

	/** Locations of graph nodes within Radius of Center that can be reached from From. */
	void GetReachableNodeLocations(const FVector& From, const FVector& Center, float Radius, const FAtlantisNavQueryFilter& Filter, TArray<FVector>& OutLocations);

	/** Nearest node with a clear line to Point, or INDEX_NONE. */
	int32 FindNearestVisibleNode(const FVector& Point);

	/** Human-readable summary for editor checks (nodes, links, components, isolated nodes, nodes inside geometry). */
	FString Describe();

	const FAtlantisNavGraph& GetGraph();

private:
	void EnsureBuilt();

	FAtlantisNavGraph Graph;
	bool bIsDirty = true;
};
