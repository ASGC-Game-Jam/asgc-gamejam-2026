// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/Navigation/AtlantisNavGraph.h"
#include "AtlantisNavGraphSubsystem.generated.h"

class APhysicsVolume;

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

	/** True if Point is inside a water volume (always true in a world without water volumes). */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Navigation")
	bool IsInWater(const FVector& Point) const;

	/** True if the whole straight line between the points stays inside water. */
	bool IsSegmentInWater(const FVector& From, const FVector& To) const;

	/**
	 * Point itself when it's in water, otherwise the closest point inside a water volume, kept an agent half
	 * height below the surface so the enemy stays submerged.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Navigation")
	FVector ClampToWater(const FVector& Point) const;

	/**
	 * Finds a 3D path between two pawn-centre locations. A goal out of the water (e.g. a player on a ledge) is
	 * moved to the closest point still underwater, so enemies wait below instead of swimming out. Uses a direct
	 * leg when the straight line is clear and underwater (and bAllowDirectLeg), otherwise enters the graph at the
	 * nearest visible node and leaves it at the node nearest the goal.
	 */
	EAtlantisNavPathResult FindPath(const FVector& From, const FVector& To, const FAtlantisNavQueryFilter& Filter, bool bAllowDirectLeg, FAtlantisNavPath& OutPath);

	/** Locations of graph nodes within Radius of Center that can be reached from From. */
	void GetReachableNodeLocations(const FVector& From, const FVector& Center, float Radius, const FAtlantisNavQueryFilter& Filter, TArray<FVector>& OutLocations);

	/** Nearest node with a clear line to Point, or INDEX_NONE. */
	int32 FindNearestVisibleNode(const FVector& Point);

	const FAtlantisNavGraph& GetGraph();

	/** Swim-only nodes left out of the last build because they weren't in water (for editor checks). */
	const TArray<FVector>& GetNodesOutsideWater() const { return NodesOutsideWater; }

private:
	void EnsureBuilt();

	/** Collects the world's water volumes (done on every rebuild). */
	void GatherWaterVolumes();

	/** Logs a warning for every patrol route point that is out of the water (enemies can't reach it). */
	void WarnAboutDryPatrolPoints() const;

	/** Clear for an enemy capsule and underwater the whole way: a straight line an enemy can actually swim. */
	bool IsSwimmable(const FVector& From, const FVector& To) const;

	/**
	 * Picks the graph nodes to enter (near From) and leave (near To) through. Prefers the nearest visible nodes;
	 * if those aren't connected, the closest visible pair that is. Returns Graph, NoStartNode or NoEndNode.
	 */
	EAtlantisNavPathResult ChooseEndpointNodes(const FVector& From, const FVector& To, int32& OutStartNode, int32& OutEndNode);

	FAtlantisNavGraph Graph;
	bool bIsDirty = true;

	TArray<TWeakObjectPtr<APhysicsVolume>> WaterVolumes;
	TArray<FVector> NodesOutsideWater;
};
