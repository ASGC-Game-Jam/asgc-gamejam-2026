// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GraphAStar.h"
#include "Core/EAtlantisTraversalMode.h"

struct FNavPathPoint;

namespace Atlantis::AI
{
	/** Bit for a traversal mode inside a mode mask. */
	constexpr uint8 TraversalModeToMask(EAtlantisTraversalMode Mode)
	{
		return static_cast<uint8>(1u << static_cast<uint8>(Mode));
	}

	/** Engine path points keep nav-link info in the low byte (bit 0x04), so the mode lives in bits 16-23. */
	constexpr uint32 EncodeTraversalModeFlags(EAtlantisTraversalMode Mode)
	{
		return static_cast<uint32>(Mode) << 16;
	}

	constexpr EAtlantisTraversalMode DecodeTraversalModeFlags(uint32 Flags)
	{
		return static_cast<EAtlantisTraversalMode>((Flags >> 16) & 0xFF);
	}
}

/** One point of a graph path. Mode is how the agent travels FROM this point to the next one. */
struct FAtlantisNavPathPoint
{
	FVector Location = FVector::ZeroVector;
	EAtlantisTraversalMode Mode = EAtlantisTraversalMode::Swimming;
};

/** A path through the nav graph, in pawn-centre space. */
struct FAtlantisNavPath
{
	TArray<FAtlantisNavPathPoint> Points;

	/**
	 * Converts to engine path points in feet space (what UPathFollowingComponent measures against),
	 * encoding each segment's traversal mode in the point flags.
	 */
	PROJECTATLANTIS_API void MakeEnginePathPoints(const FVector& CentreToFeetOffset, TArray<FNavPathPoint>& OutPoints) const;
};

struct FAtlantisNavGraphLink
{
	int32 To = INDEX_NONE;
	float Cost = 0.f;
	EAtlantisTraversalMode Mode = EAtlantisTraversalMode::Swimming;
};

struct FAtlantisNavGraphNode
{
	/** Pawn-centre location. */
	FVector Location = FVector::ZeroVector;
	/** Traversal modes allowed at this node (see Atlantis::AI::TraversalModeToMask). */
	uint8 ModeMask = 0;
	/** Connected-component id; nodes in different components can't reach each other. */
	int32 Component = INDEX_NONE;
	TArray<FAtlantisNavGraphLink> Links;
};

/** Which links a query may use. */
struct FAtlantisNavQueryFilter
{
	uint8 AllowedModes = Atlantis::AI::TraversalModeToMask(EAtlantisTraversalMode::Swimming);
};

/**
 * Plain 3D waypoint graph (no UObject, unit-testable). Nodes come from node sources; links are built from
 * a clearance test; A* runs through the engine's FGraphAStar.
 */
class PROJECTATLANTIS_API FAtlantisNavGraph
{
public:
	typedef int32 FNodeRef;
	bool IsValidRef(FNodeRef NodeRef) const { return Nodes.IsValidIndex(NodeRef); }
	int32 GetNeighbourCount(FNodeRef NodeRef) const { return Nodes[NodeRef].Links.Num(); }
	FNodeRef GetNeighbour(const FNodeRef NodeRef, const int32 NeighbourIndex) const { return Nodes[NodeRef].Links[NeighbourIndex].To; }

	void Reset();

	/** @return index of the new node. */
	int32 AddNode(const FVector& Location, uint8 ModeMask);

	/**
	 * Links every pair of nodes closer than MaxLinkDistance that share a traversal mode and pass IsClear.
	 * Links are symmetric. Also assigns connected-component ids.
	 */
	void BuildLinks(float MaxLinkDistance, TFunctionRef<bool(const FVector& From, const FVector& To)> IsClear);

	/** A* from StartNode to EndNode. OutNodes includes both ends on success. */
	EGraphAStarResult FindPath(int32 StartNode, int32 EndNode, const FAtlantisNavQueryFilter& Filter, TArray<int32>& OutNodes) const;

	/** Nodes within Radius of Center. */
	void GetNodesInRadius(const FVector& Center, float Radius, TArray<int32>& OutNodes) const;

	/** Up to MaxCount nodes, nearest first. */
	void FindNearestNodes(const FVector& Point, int32 MaxCount, TArray<int32>& OutNodes) const;

	const TArray<FAtlantisNavGraphNode>& GetNodes() const { return Nodes; }
	int32 GetNumComponents() const { return NumComponents; }
	int32 GetNumLinks() const;

	/** Link between two nodes, or nullptr. */
	const FAtlantisNavGraphLink* FindLink(int32 From, int32 To) const;

private:
	void AssignComponents();

	TArray<FAtlantisNavGraphNode> Nodes;
	int32 NumComponents = 0;
};
