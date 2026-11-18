// Copyright (c) 2026 ASGC

#include "AI/Navigation/AtlantisNavGraphLibrary.h"

#include "AI/Navigation/AtlantisNavGraphSettings.h"
#include "AI/Navigation/AtlantisNavGraphSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

FString UAtlantisNavGraphLibrary::BuildAndDescribeNavGraph(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	UAtlantisNavGraphSubsystem* NavGraph = World != nullptr ? World->GetSubsystem<UAtlantisNavGraphSubsystem>() : nullptr;
	if (NavGraph == nullptr)
	{
		return TEXT("No nav graph subsystem in this world.");
	}

	NavGraph->Rebuild();

	// Same capsule and responses as the link sweeps, so "inside geometry" matches what blocks links.
	const UAtlantisNavGraphSettings* Settings = GetDefault<UAtlantisNavGraphSettings>();
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Settings->AgentRadius, Settings->AgentHalfHeight);
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Ignore);

	const FAtlantisNavGraph& Graph = NavGraph->GetGraph();
	TArray<FString> IsolatedNodes;
	TArray<FString> BlockedNodes;
	for (const FAtlantisNavGraphNode& Node : Graph.GetNodes())
	{
		if (Node.Links.IsEmpty())
		{
			IsolatedNodes.Add(Node.Location.ToCompactString());
		}
		if (World->OverlapBlockingTestByChannel(Node.Location, FQuat::Identity, ECC_Pawn, Capsule, FCollisionQueryParams::DefaultQueryParam, ResponseParams))
		{
			BlockedNodes.Add(Node.Location.ToCompactString());
		}
	}

	TArray<FString> DryNodes;
	for (const FVector& Location : NavGraph->GetNodesOutsideWater())
	{
		DryNodes.Add(Location.ToCompactString());
	}

	return FString::Printf(TEXT("Nodes: %d | Links: %d | Components: %d | Isolated: %d [%s] | Inside geometry: %d [%s] | Outside water (left out): %d [%s]"),
		Graph.GetNodes().Num(), Graph.GetNumLinks(), Graph.GetNumComponents(),
		IsolatedNodes.Num(), *FString::Join(IsolatedNodes, TEXT("; ")),
		BlockedNodes.Num(), *FString::Join(BlockedNodes, TEXT("; ")),
		DryNodes.Num(), *FString::Join(DryNodes, TEXT("; ")));
}
