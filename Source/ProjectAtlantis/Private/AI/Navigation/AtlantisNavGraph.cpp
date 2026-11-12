// Copyright (c) 2026 ASGC

#include "AI/Navigation/AtlantisNavGraph.h"

#include "AI/Navigation/NavigationTypes.h"
#include "Algo/Find.h"

namespace
{
	/** Modes tried in order when two nodes share more than one. */
	constexpr EAtlantisTraversalMode LinkModePriority[] =
	{
		EAtlantisTraversalMode::Swimming,
		EAtlantisTraversalMode::SurfaceWalking,
		EAtlantisTraversalMode::Terrestrial,
	};

	struct FAtlantisAStarFilter
	{
		const FAtlantisNavGraph& Graph;
		const FAtlantisNavQueryFilter& Filter;

		FVector::FReal GetHeuristicScale() const { return 1.; }

		FVector::FReal GetHeuristicCost(const int32 StartNodeRef, const int32 EndNodeRef) const
		{
			return FVector::Dist(Graph.GetNodes()[StartNodeRef].Location, Graph.GetNodes()[EndNodeRef].Location);
		}

		FVector::FReal GetTraversalCost(const int32 StartNodeRef, const int32 EndNodeRef) const
		{
			const FAtlantisNavGraphLink* Link = Graph.FindLink(StartNodeRef, EndNodeRef);
			return Link != nullptr ? Link->Cost : TNumericLimits<FVector::FReal>::Max();
		}

		bool IsTraversalAllowed(const int32 NodeA, const int32 NodeB) const
		{
			const FAtlantisNavGraphLink* Link = Graph.FindLink(NodeA, NodeB);
			return Link != nullptr && (Filter.AllowedModes & Atlantis::AI::TraversalModeToMask(Link->Mode)) != 0;
		}

		bool WantsPartialSolution() const { return false; }
		bool ShouldIncludeStartNodeInPath() const { return true; }
	};
}

void FAtlantisNavPath::MakeEnginePathPoints(const FVector& CentreToFeetOffset, TArray<FNavPathPoint>& OutPoints) const
{
	OutPoints.Reset(Points.Num());
	for (const FAtlantisNavPathPoint& Point : Points)
	{
		// INVALID_NAVNODEREF and no custom nav link: the engine treats these as plain points.
		OutPoints.Emplace(Point.Location - CentreToFeetOffset, INVALID_NAVNODEREF, Atlantis::AI::EncodeTraversalModeFlags(Point.Mode));
	}
}

void FAtlantisNavGraph::Reset()
{
	Nodes.Reset();
	NumComponents = 0;
}

int32 FAtlantisNavGraph::AddNode(const FVector& Location, uint8 ModeMask)
{
	FAtlantisNavGraphNode& Node = Nodes.AddDefaulted_GetRef();
	Node.Location = Location;
	Node.ModeMask = ModeMask;
	return Nodes.Num() - 1;
}

void FAtlantisNavGraph::BuildLinks(float MaxLinkDistance, TFunctionRef<bool(const FVector& From, const FVector& To)> IsClear)
{
	const float MaxLinkDistanceSq = FMath::Square(MaxLinkDistance);

	for (FAtlantisNavGraphNode& Node : Nodes)
	{
		Node.Links.Reset();
	}

	for (int32 A = 0; A < Nodes.Num(); ++A)
	{
		for (int32 B = A + 1; B < Nodes.Num(); ++B)
		{
			const FVector& LocationA = Nodes[A].Location;
			const FVector& LocationB = Nodes[B].Location;
			if (FVector::DistSquared(LocationA, LocationB) > MaxLinkDistanceSq)
			{
				continue;
			}

			const uint8 SharedModes = Nodes[A].ModeMask & Nodes[B].ModeMask;
			const EAtlantisTraversalMode* LinkMode = Algo::FindByPredicate(LinkModePriority, [SharedModes](EAtlantisTraversalMode Mode)
			{
				return (SharedModes & Atlantis::AI::TraversalModeToMask(Mode)) != 0;
			});
			if (LinkMode == nullptr || !IsClear(LocationA, LocationB))
			{
				continue;
			}

			const float Cost = FVector::Dist(LocationA, LocationB);
			Nodes[A].Links.Add({B, Cost, *LinkMode});
			Nodes[B].Links.Add({A, Cost, *LinkMode});
		}
	}

	AssignComponents();
}

void FAtlantisNavGraph::AssignComponents()
{
	NumComponents = 0;
	for (FAtlantisNavGraphNode& Node : Nodes)
	{
		Node.Component = INDEX_NONE;
	}

	TArray<int32> Stack;
	for (int32 Seed = 0; Seed < Nodes.Num(); ++Seed)
	{
		if (Nodes[Seed].Component != INDEX_NONE)
		{
			continue;
		}

		const int32 ComponentId = NumComponents++;
		Nodes[Seed].Component = ComponentId;
		Stack.Reset();
		Stack.Push(Seed);
		while (Stack.Num() > 0)
		{
			const int32 Current = Stack.Pop(EAllowShrinking::No);
			for (const FAtlantisNavGraphLink& Link : Nodes[Current].Links)
			{
				if (Nodes[Link.To].Component == INDEX_NONE)
				{
					Nodes[Link.To].Component = ComponentId;
					Stack.Push(Link.To);
				}
			}
		}
	}
}

EGraphAStarResult FAtlantisNavGraph::FindPath(int32 StartNode, int32 EndNode, const FAtlantisNavQueryFilter& Filter, TArray<int32>& OutNodes) const
{
	OutNodes.Reset();
	if (!IsValidRef(StartNode) || !IsValidRef(EndNode))
	{
		return EGraphAStarResult::SearchFail;
	}

	// Different islands can never connect; skip the search.
	if (Nodes[StartNode].Component != Nodes[EndNode].Component)
	{
		return EGraphAStarResult::GoalUnreachable;
	}

	FGraphAStar<FAtlantisNavGraph> Pathfinder(*this);
	return Pathfinder.FindPath(StartNode, EndNode, FAtlantisAStarFilter{*this, Filter}, OutNodes);
}

void FAtlantisNavGraph::GetNodesInRadius(const FVector& Center, float Radius, TArray<int32>& OutNodes) const
{
	OutNodes.Reset();
	const float RadiusSq = FMath::Square(Radius);
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		if (FVector::DistSquared(Nodes[Index].Location, Center) <= RadiusSq)
		{
			OutNodes.Add(Index);
		}
	}
}

void FAtlantisNavGraph::FindNearestNodes(const FVector& Point, int32 MaxCount, TArray<int32>& OutNodes) const
{
	OutNodes.Reset(Nodes.Num());
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		OutNodes.Add(Index);
	}

	OutNodes.Sort([this, &Point](const int32 A, const int32 B)
	{
		return FVector::DistSquared(Nodes[A].Location, Point) < FVector::DistSquared(Nodes[B].Location, Point);
	});

	if (MaxCount >= 0 && OutNodes.Num() > MaxCount)
	{
		OutNodes.SetNum(MaxCount);
	}
}

int32 FAtlantisNavGraph::GetNumLinks() const
{
	int32 NumLinks = 0;
	for (const FAtlantisNavGraphNode& Node : Nodes)
	{
		NumLinks += Node.Links.Num();
	}
	// Links are stored on both ends.
	return NumLinks / 2;
}

const FAtlantisNavGraphLink* FAtlantisNavGraph::FindLink(int32 From, int32 To) const
{
	if (!IsValidRef(From))
	{
		return nullptr;
	}
	return Nodes[From].Links.FindByPredicate([To](const FAtlantisNavGraphLink& Link) { return Link.To == To; });
}
