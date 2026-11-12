// Copyright (c) 2026 ASGC

#include "AI/Navigation/AtlantisNavGraphDebug.h"

#include "AI/Navigation/AtlantisNavGraph.h"
#include "AIController.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Navigation/PathFollowingComponent.h"

namespace
{
	enum class EDrawMode : int32
	{
		Off = 0,
		AgentPaths = 1,
		Nodes = 2,
		NodesAndLinksNearCamera = 3
	};

	int32 GAtlantisNavGraphDraw = 0;
	FAutoConsoleVariableRef CVarAtlantisNavGraphDraw(
		TEXT("Atlantis.AI.NavGraph.Draw"),
		GAtlantisNavGraphDraw,
		TEXT("Swimming AI nav graph debug view. 0 = off, 1 = each AI's current path, 2 = nodes only (red = no links), ")
		TEXT("3 = nodes and links within Atlantis.AI.NavGraph.DrawRadius of the camera."),
		ECVF_Cheat);

	float GAtlantisNavGraphDrawRadius = 1500.f;
	FAutoConsoleVariableRef CVarAtlantisNavGraphDrawRadius(
		TEXT("Atlantis.AI.NavGraph.DrawRadius"),
		GAtlantisNavGraphDrawRadius,
		TEXT("Radius (cm) around the camera drawn by Atlantis.AI.NavGraph.Draw 3."),
		ECVF_Cheat);

	const FColor ComponentColors[] = {FColor::Cyan, FColor::Green, FColor::Yellow, FColor::Magenta, FColor::Orange, FColor::Turquoise};
	const FColor AgentColors[] = {FColor::Orange, FColor::Cyan, FColor::Magenta, FColor::Green, FColor::Yellow, FColor::Emerald};

	constexpr float NodeRadius = 15.f;

	FColor GetComponentColor(const FAtlantisNavGraphNode& Node)
	{
		return Node.Links.IsEmpty() ? FColor::Red : ComponentColors[FMath::Max(Node.Component, 0) % UE_ARRAY_COUNT(ComponentColors)];
	}

	/** Each AI that is following a path: the rest of the path, a sphere on the point it's heading to, and its goal. */
	void DrawAgentPaths(const UWorld& World)
	{
		for (TActorIterator<AAIController> It(&World); It; ++It)
		{
			const AAIController* Controller = *It;
			const APawn* Pawn = Controller->GetPawn();
			const UPathFollowingComponent* PathFollowing = Controller->GetPathFollowingComponent();
			if (Pawn == nullptr || PathFollowing == nullptr || PathFollowing->GetStatus() == EPathFollowingStatus::Idle)
			{
				continue;
			}

			const FNavPathSharedPtr Path = PathFollowing->GetPath();
			if (!Path.IsValid() || Path->GetPathPoints().Num() < 2)
			{
				continue;
			}

			// Stable colour per AI, so several guards can be told apart.
			const FColor Color = AgentColors[GetTypeHash(Controller->GetFName()) % UE_ARRAY_COUNT(AgentColors)];
			const TArray<FNavPathPoint>& Points = Path->GetPathPoints();
			const int32 NextIndex = FMath::Min(static_cast<int32>(PathFollowing->GetCurrentPathIndex()) + 1, Points.Num() - 1);

			DrawDebugLine(&World, Pawn->GetActorLocation(), Points[NextIndex].Location, Color, false, -1.f, 0, 4.f);
			for (int32 Index = NextIndex; Index < Points.Num() - 1; ++Index)
			{
				DrawDebugLine(&World, Points[Index].Location, Points[Index + 1].Location, Color, false, -1.f, 0, 2.f);
			}
			DrawDebugSphere(&World, Points[NextIndex].Location, 30.f, 8, Color, false, -1.f, 0, 2.f);

			const FVector Goal = Points.Last().Location;
			DrawDebugCrosshairs(&World, Goal, FRotator::ZeroRotator, 80.f, Color);
			DrawDebugString(&World, Goal + FVector(0.f, 0.f, 60.f), Pawn->GetName(), nullptr, Color, 0.f, true);
		}
	}

	void DrawNodes(const UWorld& World, const FAtlantisNavGraph& Graph)
	{
		for (const FAtlantisNavGraphNode& Node : Graph.GetNodes())
		{
			DrawDebugSphere(&World, Node.Location, NodeRadius, 6, GetComponentColor(Node));
		}
	}

	/** Nodes and links around the camera only, links thin and dimmed so the nodes stay readable. */
	void DrawNearCamera(const UWorld& World, const FAtlantisNavGraph& Graph)
	{
		const APlayerController* Player = World.GetFirstPlayerController();
		if (Player == nullptr)
		{
			return;
		}

		FVector ViewLocation;
		FRotator ViewRotation;
		Player->GetPlayerViewPoint(ViewLocation, ViewRotation);
		const double RadiusSq = FMath::Square(static_cast<double>(GAtlantisNavGraphDrawRadius));

		const TArray<FAtlantisNavGraphNode>& Nodes = Graph.GetNodes();
		for (int32 Index = 0; Index < Nodes.Num(); ++Index)
		{
			const FAtlantisNavGraphNode& Node = Nodes[Index];
			if (FVector::DistSquared(ViewLocation, Node.Location) > RadiusSq)
			{
				continue;
			}

			const FColor Color = GetComponentColor(Node);
			DrawDebugSphere(&World, Node.Location, NodeRadius, 6, Color);

			const FColor LinkColor(Color.R / 3, Color.G / 3, Color.B / 3);
			for (const FAtlantisNavGraphLink& Link : Node.Links)
			{
				// Each link is stored on both ends: draw it once, from the lower index (or from this end if the other is out of range).
				const bool bOtherInRange = FVector::DistSquared(ViewLocation, Nodes[Link.To].Location) <= RadiusSq;
				if (bOtherInRange && Link.To < Index)
				{
					continue;
				}
				DrawDebugLine(&World, Node.Location, Nodes[Link.To].Location, LinkColor, false, -1.f, 0, 0.f);
			}
		}
	}
}

bool Atlantis::AI::NavGraphDebug::IsDrawEnabled()
{
	return GAtlantisNavGraphDraw > 0;
}

void Atlantis::AI::NavGraphDebug::DrawGraph(const UWorld& World, const FAtlantisNavGraph& Graph)
{
#if ENABLE_DRAW_DEBUG
	switch (static_cast<EDrawMode>(GAtlantisNavGraphDraw))
	{
	case EDrawMode::AgentPaths:
		DrawAgentPaths(World);
		break;
	case EDrawMode::Nodes:
		DrawNodes(World, Graph);
		break;
	case EDrawMode::NodesAndLinksNearCamera:
		DrawNearCamera(World, Graph);
		break;
	default:
		break;
	}
#endif // ENABLE_DRAW_DEBUG
}
