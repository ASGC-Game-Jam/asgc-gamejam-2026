// Copyright (c) 2026 ASGC

#include "AI/Navigation/AtlantisNavGraphSubsystem.h"

#include "AI/AtlantisAITypes.h"
#include "AI/Navigation/AtlantisNavGraphDebug.h"
#include "AI/Navigation/AtlantisNavGraphSettings.h"
#include "AI/Navigation/AtlantisNavNodeSource.h"
#include "AI/Patrol/AtlantisPatrolRoute.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DefaultPhysicsVolume.h"
#include "GameFramework/PhysicsVolume.h"

namespace
{
	/** Spacing of the in-water samples along a line that crosses between water volumes. */
	constexpr double WaterSampleSpacing = 100.0;
}

void UAtlantisNavGraphSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UAtlantisNavGraphSubsystem::Deinitialize()
{
	Graph.Reset();
	Super::Deinitialize();
}

void UAtlantisNavGraphSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	Rebuild();
	WarnAboutDryPatrolPoints();
}

bool UAtlantisNavGraphSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// Editor worlds are supported so the graph can be checked from editor scripts.
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE || WorldType == EWorldType::Editor;
}

void UAtlantisNavGraphSubsystem::Rebuild()
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	TArray<FAtlantisNavNodeDesc> NodeDescs;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (const IAtlantisNavNodeSource* Source = Cast<IAtlantisNavNodeSource>(*It))
		{
			Source->GatherNavNodes(NodeDescs);
		}
	}

	GatherWaterVolumes();

	// Swim-only nodes above the surface can never be reached: leave them out and report them.
	const uint8 SwimOnlyMask = Atlantis::AI::TraversalModeToMask(EAtlantisTraversalMode::Swimming);
	Graph.Reset();
	NodesOutsideWater.Reset();
	for (const FAtlantisNavNodeDesc& Desc : NodeDescs)
	{
		if (Desc.ModeMask == SwimOnlyMask && !IsInWater(Desc.Location))
		{
			NodesOutsideWater.Add(Desc.Location);
			continue;
		}
		Graph.AddNode(Desc.Location, Desc.ModeMask);
	}
	if (!NodesOutsideWater.IsEmpty())
	{
		UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] %d swim nav node(s) are out of the water and were left out of the graph (first at %s)."),
			__FUNCTION__, NodesOutsideWater.Num(), *NodesOutsideWater[0].ToCompactString());
	}

	const UAtlantisNavGraphSettings* Settings = GetDefault<UAtlantisNavGraphSettings>();
	Graph.BuildLinks(Settings->MaxLinkDistance, [this](const FVector& From, const FVector& To)
	{
		return IsSwimmable(From, To);
	});

	bIsDirty = false;
	UE_LOG(LogAtlantisAI, Log, TEXT("[%hs] Swim nav graph built: %d nodes, %d links, %d components."),
		__FUNCTION__, Graph.GetNodes().Num(), Graph.GetNumLinks(), Graph.GetNumComponents());
}

void UAtlantisNavGraphSubsystem::EnsureBuilt()
{
	if (bIsDirty)
	{
		Rebuild();
	}
}

const FAtlantisNavGraph& UAtlantisNavGraphSubsystem::GetGraph()
{
	EnsureBuilt();
	return Graph;
}

bool UAtlantisNavGraphSubsystem::IsSegmentClear(const FVector& From, const FVector& To) const
{
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	const UAtlantisNavGraphSettings* Settings = GetDefault<UAtlantisNavGraphSettings>();
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Settings->AgentRadius, Settings->AgentHalfHeight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AtlantisNavGraphSweep), false);
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

	return !World->SweepTestByChannel(From, To, FQuat::Identity, ECC_Pawn, Capsule, QueryParams, ResponseParams);
}

void UAtlantisNavGraphSubsystem::GatherWaterVolumes()
{
	WaterVolumes.Reset();
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<APhysicsVolume> It(World); It; ++It)
		{
			if (It->bWaterVolume && !It->IsA<ADefaultPhysicsVolume>())
			{
				WaterVolumes.Add(*It);
			}
		}
	}
}

bool UAtlantisNavGraphSubsystem::IsInWater(const FVector& Point) const
{
	if (WaterVolumes.IsEmpty())
	{
		return true;
	}
	for (const TWeakObjectPtr<APhysicsVolume>& Volume : WaterVolumes)
	{
		if (Volume.IsValid() && Volume->EncompassesPoint(Point))
		{
			return true;
		}
	}
	return false;
}

bool UAtlantisNavGraphSubsystem::IsSegmentInWater(const FVector& From, const FVector& To) const
{
	if (WaterVolumes.IsEmpty())
	{
		return true;
	}

	// Water volumes are convex boxes: both ends inside the same one means the whole line is.
	for (const TWeakObjectPtr<APhysicsVolume>& Volume : WaterVolumes)
	{
		if (Volume.IsValid() && Volume->EncompassesPoint(From) && Volume->EncompassesPoint(To))
		{
			return true;
		}
	}

	// Crossing between volumes (or leaving them): sample along the line.
	const int32 NumSteps = FMath::Max(1, FMath::CeilToInt(FVector::Dist(From, To) / WaterSampleSpacing));
	for (int32 Step = 0; Step <= NumSteps; ++Step)
	{
		if (!IsInWater(FMath::Lerp(From, To, static_cast<double>(Step) / NumSteps)))
		{
			return false;
		}
	}
	return true;
}

FVector UAtlantisNavGraphSubsystem::ClampToWater(const FVector& Point) const
{
	if (IsInWater(Point))
	{
		return Point;
	}

	// Closest point inside a water volume's bounds, shrunk by the agent size so the capsule stays submerged.
	const UAtlantisNavGraphSettings* Settings = GetDefault<UAtlantisNavGraphSettings>();
	const FVector Margin(Settings->AgentRadius, Settings->AgentRadius, Settings->AgentHalfHeight);
	FVector Best = Point;
	double BestDistanceSq = TNumericLimits<double>::Max();
	for (const TWeakObjectPtr<APhysicsVolume>& Volume : WaterVolumes)
	{
		if (!Volume.IsValid())
		{
			continue;
		}
		FBox Box = Volume->GetComponentsBoundingBox(true);
		const FVector Extent = Box.GetExtent();
		Box = Box.ExpandBy(-FVector(FMath::Min(Margin.X, Extent.X), FMath::Min(Margin.Y, Extent.Y), FMath::Min(Margin.Z, Extent.Z)));
		const FVector Candidate = Box.GetClosestPointTo(Point);
		const double DistanceSq = FVector::DistSquared(Point, Candidate);
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			Best = Candidate;
		}
	}
	return Best;
}

bool UAtlantisNavGraphSubsystem::IsSwimmable(const FVector& From, const FVector& To) const
{
	// Ends are snapped below the surface first: a pawn swimming at the surface has its centre just above it.
	return IsSegmentClear(From, To) && IsSegmentInWater(ClampToWater(From), ClampToWater(To));
}

void UAtlantisNavGraphSubsystem::WarnAboutDryPatrolPoints() const
{
	const UWorld* World = GetWorld();
	if (World == nullptr || WaterVolumes.IsEmpty())
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->Implements<UAtlantisPatrolRoute>())
		{
			continue;
		}
		const USplineComponent* Spline = IAtlantisPatrolRoute::Execute_GetRouteSpline(*It);
		if (Spline == nullptr)
		{
			continue;
		}
		for (int32 Index = 0; Index < Spline->GetNumberOfSplinePoints(); ++Index)
		{
			const FVector Location = Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World);
			if (!IsInWater(Location))
			{
				UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] %s: patrol point %d at %s is out of the water; swimming enemies will stop below it."),
					__FUNCTION__, *It->GetName(), Index, *Location.ToCompactString());
			}
		}
	}
}

int32 UAtlantisNavGraphSubsystem::FindNearestVisibleNode(const FVector& Point)
{
	EnsureBuilt();

	TArray<int32> Candidates;
	Graph.FindNearestNodes(Point, GetDefault<UAtlantisNavGraphSettings>()->MaxNearestCandidates, Candidates);
	for (const int32 Candidate : Candidates)
	{
		if (IsSwimmable(Point, Graph.GetNodes()[Candidate].Location))
		{
			return Candidate;
		}
	}
	return INDEX_NONE;
}

EAtlantisNavPathResult UAtlantisNavGraphSubsystem::ChooseEndpointNodes(const FVector& From, const FVector& To, int32& OutStartNode, int32& OutEndNode)
{
	OutStartNode = FindNearestVisibleNode(From);
	if (OutStartNode == INDEX_NONE)
	{
		return EAtlantisNavPathResult::NoStartNode;
	}

	OutEndNode = FindNearestVisibleNode(To);
	if (OutEndNode == INDEX_NONE)
	{
		return EAtlantisNavPathResult::NoEndNode;
	}

	// The nearest nodes can sit in different components (e.g. one is an isolated node) even when another
	// visible pair connects. Only then pay for checking every visible candidate and take the closest pair
	// that shares a component.
	const TArray<FAtlantisNavGraphNode>& Nodes = Graph.GetNodes();
	if (Nodes[OutStartNode].Component == Nodes[OutEndNode].Component)
	{
		return EAtlantisNavPathResult::Graph;
	}

	const int32 MaxCandidates = GetDefault<UAtlantisNavGraphSettings>()->MaxNearestCandidates;
	auto GatherVisible = [this, &Nodes, MaxCandidates](const FVector& Point, TArray<int32>& OutVisible)
	{
		TArray<int32> Candidates;
		Graph.FindNearestNodes(Point, MaxCandidates, Candidates);
		for (const int32 Candidate : Candidates)
		{
			if (IsSwimmable(Point, Nodes[Candidate].Location))
			{
				OutVisible.Add(Candidate);
			}
		}
	};

	TArray<int32> StartCandidates;
	TArray<int32> EndCandidates;
	GatherVisible(From, StartCandidates);
	GatherVisible(To, EndCandidates);

	double BestDistance = TNumericLimits<double>::Max();
	for (const int32 StartCandidate : StartCandidates)
	{
		for (const int32 EndCandidate : EndCandidates)
		{
			if (Nodes[StartCandidate].Component != Nodes[EndCandidate].Component)
			{
				continue;
			}
			const double Distance = FVector::Dist(From, Nodes[StartCandidate].Location) + FVector::Dist(To, Nodes[EndCandidate].Location);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				OutStartNode = StartCandidate;
				OutEndNode = EndCandidate;
			}
		}
	}

	// No connected pair: the nearest nodes are kept and A* reports the goal unreachable.
	return EAtlantisNavPathResult::Graph;
}

EAtlantisNavPathResult UAtlantisNavGraphSubsystem::FindPath(const FVector& From, const FVector& To, const FAtlantisNavQueryFilter& Filter, bool bAllowDirectLeg, FAtlantisNavPath& OutPath)
{
	OutPath.Points.Reset();

	// The legs into and out of the graph, and the direct leg, are always swum.
	constexpr EAtlantisTraversalMode OpenWaterMode = EAtlantisTraversalMode::Swimming;

	// A goal out of the water (player on a ledge or at the surface): wait at the closest point still underwater.
	const FVector Goal = ClampToWater(To);

	if (bAllowDirectLeg && IsSwimmable(From, Goal))
	{
		OutPath.Points.Add({From, OpenWaterMode});
		OutPath.Points.Add({Goal, OpenWaterMode});
		return EAtlantisNavPathResult::Direct;
	}

	int32 StartNode = INDEX_NONE;
	int32 EndNode = INDEX_NONE;
	const EAtlantisNavPathResult EndpointResult = ChooseEndpointNodes(From, Goal, StartNode, EndNode);
	if (EndpointResult != EAtlantisNavPathResult::Graph)
	{
		return EndpointResult;
	}

	TArray<int32> NodePath;
	if (Graph.FindPath(StartNode, EndNode, Filter, NodePath) != EGraphAStarResult::SearchSuccess)
	{
		return EAtlantisNavPathResult::Unreachable;
	}

	const TArray<FAtlantisNavGraphNode>& Nodes = Graph.GetNodes();
	OutPath.Points.Add({From, OpenWaterMode});
	for (int32 Index = 0; Index < NodePath.Num(); ++Index)
	{
		const bool bIsLastNode = Index == NodePath.Num() - 1;
		const FAtlantisNavGraphLink* LinkToNext = bIsLastNode ? nullptr : Graph.FindLink(NodePath[Index], NodePath[Index + 1]);
		OutPath.Points.Add({Nodes[NodePath[Index]].Location, LinkToNext != nullptr ? LinkToNext->Mode : OpenWaterMode});
	}
	OutPath.Points.Add({Goal, OpenWaterMode});
	return EAtlantisNavPathResult::Graph;
}

void UAtlantisNavGraphSubsystem::GetReachableNodeLocations(const FVector& From, const FVector& Center, float Radius, const FAtlantisNavQueryFilter& Filter, TArray<FVector>& OutLocations)
{
	OutLocations.Reset();

	// Enter the graph on the side connected to Center when possible, so an isolated node next to the
	// enemy doesn't hide every search point. Without a node visible from Center, use the one nearest From.
	int32 FromNode = INDEX_NONE;
	int32 CenterNode = INDEX_NONE;
	if (ChooseEndpointNodes(From, ClampToWater(Center), FromNode, CenterNode) == EAtlantisNavPathResult::NoStartNode)
	{
		return;
	}

	TArray<int32> NodesInRadius;
	Graph.GetNodesInRadius(Center, Radius, NodesInRadius);

	const TArray<FAtlantisNavGraphNode>& Nodes = Graph.GetNodes();
	for (const int32 Node : NodesInRadius)
	{
		if (Nodes[Node].Component == Nodes[FromNode].Component && (Nodes[Node].ModeMask & Filter.AllowedModes) != 0)
		{
			OutLocations.Add(Nodes[Node].Location);
		}
	}
}

bool UAtlantisNavGraphSubsystem::IsTickable() const
{
	return Atlantis::AI::NavGraphDebug::IsDrawEnabled() && IsInitialized();
}

TStatId UAtlantisNavGraphSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAtlantisNavGraphSubsystem, STATGROUP_Tickables);
}

void UAtlantisNavGraphSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (const UWorld* World = GetWorld())
	{
		Atlantis::AI::NavGraphDebug::DrawGraph(*World, Graph);
	}
}
