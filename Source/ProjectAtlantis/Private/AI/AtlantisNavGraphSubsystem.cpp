// Copyright (c) 2026 ASGC

#include "AI/AtlantisNavGraphSubsystem.h"

#include "AI/AtlantisAITypes.h"
#include "AI/AtlantisNavGraphSettings.h"
#include "AI/AtlantisNavNodeSource.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"

namespace
{
	int32 GAtlantisNavGraphDraw = 0;
	FAutoConsoleVariableRef CVarAtlantisNavGraphDraw(
		TEXT("Atlantis.AI.NavGraph.Draw"),
		GAtlantisNavGraphDraw,
		TEXT("Draw the swimming AI nav graph. 0 = off, 1 = nodes and links."),
		ECVF_Cheat);

	const FColor ComponentColors[] = {FColor::Cyan, FColor::Green, FColor::Yellow, FColor::Magenta, FColor::Orange, FColor::Turquoise};
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

	Graph.Reset();
	for (const FAtlantisNavNodeDesc& Desc : NodeDescs)
	{
		Graph.AddNode(Desc.Location, Desc.ModeMask);
	}

	const UAtlantisNavGraphSettings* Settings = GetDefault<UAtlantisNavGraphSettings>();
	Graph.BuildLinks(Settings->MaxLinkDistance, [this](const FVector& From, const FVector& To)
	{
		return IsSegmentClear(From, To);
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
	// Players and other enemies must not cut links; water volumes only overlap, so they never block.
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

	return !World->SweepTestByChannel(From, To, FQuat::Identity, ECC_Pawn, Capsule, QueryParams, ResponseParams);
}

int32 UAtlantisNavGraphSubsystem::FindNearestVisibleNode(const FVector& Point)
{
	EnsureBuilt();

	TArray<int32> Candidates;
	Graph.FindNearestNodes(Point, GetDefault<UAtlantisNavGraphSettings>()->MaxNearestCandidates, Candidates);
	for (const int32 Candidate : Candidates)
	{
		if (IsSegmentClear(Point, Graph.GetNodes()[Candidate].Location))
		{
			return Candidate;
		}
	}
	return INDEX_NONE;
}

EAtlantisNavPathResult UAtlantisNavGraphSubsystem::FindPath(const FVector& From, const FVector& To, const FAtlantisNavQueryFilter& Filter, bool bAllowDirectLeg, FAtlantisNavPath& OutPath)
{
	OutPath.Points.Reset();

	// The legs into and out of the graph, and the direct leg, are always swum.
	constexpr EAtlantisTraversalMode OpenWaterMode = EAtlantisTraversalMode::Swimming;

	if (bAllowDirectLeg && IsSegmentClear(From, To))
	{
		OutPath.Points.Add({From, OpenWaterMode});
		OutPath.Points.Add({To, OpenWaterMode});
		return EAtlantisNavPathResult::Direct;
	}

	const int32 StartNode = FindNearestVisibleNode(From);
	if (StartNode == INDEX_NONE)
	{
		return EAtlantisNavPathResult::NoStartNode;
	}

	const int32 EndNode = FindNearestVisibleNode(To);
	if (EndNode == INDEX_NONE)
	{
		return EAtlantisNavPathResult::NoEndNode;
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
	OutPath.Points.Add({To, OpenWaterMode});
	return EAtlantisNavPathResult::Graph;
}

void UAtlantisNavGraphSubsystem::GetReachableNodeLocations(const FVector& From, const FVector& Center, float Radius, const FAtlantisNavQueryFilter& Filter, TArray<FVector>& OutLocations)
{
	OutLocations.Reset();

	const int32 FromNode = FindNearestVisibleNode(From);
	if (FromNode == INDEX_NONE)
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

FString UAtlantisNavGraphSubsystem::Describe()
{
	Rebuild();

	const UWorld* World = GetWorld();
	const UAtlantisNavGraphSettings* Settings = GetDefault<UAtlantisNavGraphSettings>();
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Settings->AgentRadius, Settings->AgentHalfHeight);
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

	TArray<FString> IsolatedNodes;
	TArray<FString> BlockedNodes;
	const TArray<FAtlantisNavGraphNode>& Nodes = Graph.GetNodes();
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		if (Nodes[Index].Links.IsEmpty())
		{
			IsolatedNodes.Add(Nodes[Index].Location.ToCompactString());
		}
		if (World != nullptr && World->OverlapBlockingTestByChannel(Nodes[Index].Location, FQuat::Identity, ECC_Pawn, Capsule, FCollisionQueryParams::DefaultQueryParam, ResponseParams))
		{
			BlockedNodes.Add(Nodes[Index].Location.ToCompactString());
		}
	}

	return FString::Printf(TEXT("Nodes: %d | Links: %d | Components: %d | Isolated: %d [%s] | Inside geometry: %d [%s]"),
		Nodes.Num(), Graph.GetNumLinks(), Graph.GetNumComponents(),
		IsolatedNodes.Num(), *FString::Join(IsolatedNodes, TEXT("; ")),
		BlockedNodes.Num(), *FString::Join(BlockedNodes, TEXT("; ")));
}

bool UAtlantisNavGraphSubsystem::IsTickable() const
{
	return GAtlantisNavGraphDraw > 0 && IsInitialized();
}

TStatId UAtlantisNavGraphSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAtlantisNavGraphSubsystem, STATGROUP_Tickables);
}

void UAtlantisNavGraphSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

#if ENABLE_DRAW_DEBUG
	const UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const TArray<FAtlantisNavGraphNode>& Nodes = Graph.GetNodes();
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		const FAtlantisNavGraphNode& Node = Nodes[Index];
		const FColor Color = ComponentColors[FMath::Max(Node.Component, 0) % UE_ARRAY_COUNT(ComponentColors)];
		DrawDebugSphere(World, Node.Location, 20.f, 8, Node.Links.IsEmpty() ? FColor::Red : Color);
		for (const FAtlantisNavGraphLink& Link : Node.Links)
		{
			// Each link is stored on both ends; draw it once.
			if (Link.To > Index)
			{
				DrawDebugLine(World, Node.Location, Nodes[Link.To].Location, Color, false, -1.f, 0, 2.f);
			}
		}
	}
#endif // ENABLE_DRAW_DEBUG
}
