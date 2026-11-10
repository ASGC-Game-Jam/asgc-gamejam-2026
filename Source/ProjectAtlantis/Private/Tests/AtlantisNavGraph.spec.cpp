// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/AtlantisNavGraph.h"
#include "AI/Navigation/NavigationTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	/** Fake wall: the plane X = 0 for |Y| < 400. Segments crossing it there are blocked. */
	bool IsClearOfTestWall(const FVector& From, const FVector& To)
	{
		if ((From.X < 0.) == (To.X < 0.) || FMath::IsNearlyEqual(From.X, To.X))
		{
			return true;
		}
		const double T = -From.X / (To.X - From.X);
		const double CrossingY = From.Y + T * (To.Y - From.Y);
		return FMath::Abs(CrossingY) >= 400.;
	}

	constexpr uint8 SwimMask = Atlantis::AI::TraversalModeToMask(EAtlantisTraversalMode::Swimming);
}

BEGIN_DEFINE_SPEC(FAtlantisNavGraphSpec, "Atlantis.AI.Nav.Graph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FAtlantisNavGraph Graph;
END_DEFINE_SPEC(FAtlantisNavGraphSpec)

void FAtlantisNavGraphSpec::Define()
{
	BeforeEach([this]()
	{
		Graph.Reset();
	});

	It("routes around a wall instead of through it", [this]()
	{
		const int32 Start = Graph.AddNode(FVector(-500, 0, 0), SwimMask);
		const int32 Goal = Graph.AddNode(FVector(500, 0, 0), SwimMask);
		const int32 DetourA = Graph.AddNode(FVector(-500, 600, 0), SwimMask);
		const int32 DetourB = Graph.AddNode(FVector(500, 600, 0), SwimMask);
		Graph.BuildLinks(1200.f, IsClearOfTestWall);

		TestNull(TEXT("No link straight through the wall"), Graph.FindLink(Start, Goal));

		TArray<int32> Path;
		TestTrue(TEXT("Path found"), Graph.FindPath(Start, Goal, FAtlantisNavQueryFilter(), Path) == EGraphAStarResult::SearchSuccess);
		TestTrue(TEXT("Path goes through both detour nodes"), Path == TArray<int32>{Start, DetourA, DetourB, Goal});
	});

	It("keeps links symmetric and within the max distance", [this]()
	{
		const int32 A = Graph.AddNode(FVector(0, 0, 0), SwimMask);
		const int32 B = Graph.AddNode(FVector(0, 0, 900), SwimMask);
		const int32 Far = Graph.AddNode(FVector(0, 0, 5000), SwimMask);
		Graph.BuildLinks(1000.f, [](const FVector&, const FVector&) { return true; });

		TestNotNull(TEXT("A->B linked (vertical links count in 3D)"), Graph.FindLink(A, B));
		TestNotNull(TEXT("B->A linked"), Graph.FindLink(B, A));
		TestNull(TEXT("Far node not linked"), Graph.FindLink(B, Far));
		TestEqual(TEXT("One link total"), Graph.GetNumLinks(), 1);
	});

	It("reports unreachable goals in separate components", [this]()
	{
		const int32 A = Graph.AddNode(FVector(0, 0, 0), SwimMask);
		const int32 Island = Graph.AddNode(FVector(10000, 0, 0), SwimMask);
		Graph.BuildLinks(1000.f, [](const FVector&, const FVector&) { return true; });

		TestEqual(TEXT("Two components"), Graph.GetNumComponents(), 2);
		TArray<int32> Path;
		TestTrue(TEXT("Unreachable"), Graph.FindPath(A, Island, FAtlantisNavQueryFilter(), Path) == EGraphAStarResult::GoalUnreachable);
	});

	It("does not use links whose mode the filter excludes", [this]()
	{
		const int32 A = Graph.AddNode(FVector(0, 0, 0), SwimMask);
		const int32 B = Graph.AddNode(FVector(500, 0, 0), SwimMask);
		Graph.BuildLinks(1000.f, [](const FVector&, const FVector&) { return true; });

		FAtlantisNavQueryFilter WalkOnly;
		WalkOnly.AllowedModes = Atlantis::AI::TraversalModeToMask(EAtlantisTraversalMode::SurfaceWalking);
		TArray<int32> Path;
		TestTrue(TEXT("No path for a filter without Swimming"), Graph.FindPath(A, B, WalkOnly, Path) != EGraphAStarResult::SearchSuccess);
	});

	It("does not link nodes that share no traversal mode", [this]()
	{
		const int32 Swim = Graph.AddNode(FVector(0, 0, 0), SwimMask);
		const int32 Walk = Graph.AddNode(FVector(100, 0, 0), Atlantis::AI::TraversalModeToMask(EAtlantisTraversalMode::SurfaceWalking));
		Graph.BuildLinks(1000.f, [](const FVector&, const FVector&) { return true; });
		TestNull(TEXT("No link"), Graph.FindLink(Swim, Walk));
	});

	It("finds nodes in a radius and nearest nodes in order", [this]()
	{
		Graph.AddNode(FVector(0, 0, 0), SwimMask);
		const int32 Near = Graph.AddNode(FVector(0, 0, 300), SwimMask);
		Graph.AddNode(FVector(0, 0, 2000), SwimMask);

		TArray<int32> InRadius;
		Graph.GetNodesInRadius(FVector(0, 0, 250), 400.f, InRadius);
		TestEqual(TEXT("Two nodes in radius"), InRadius.Num(), 2);

		TArray<int32> Nearest;
		Graph.FindNearestNodes(FVector(0, 0, 290), 2, Nearest);
		TestEqual(TEXT("Capped at 2"), Nearest.Num(), 2);
		TestEqual(TEXT("Nearest first"), Nearest[0], Near);
	});

	It("encodes traversal modes into engine path points without touching the nav-link bit", [this]()
	{
		FAtlantisNavPath Path;
		Path.Points.Add({FVector(0, 0, 100), EAtlantisTraversalMode::Swimming});
		Path.Points.Add({FVector(0, 0, 500), EAtlantisTraversalMode::SurfaceWalking});

		TArray<FNavPathPoint> EnginePoints;
		Path.MakeEnginePathPoints(FVector(0, 0, 90), EnginePoints);

		TestEqual(TEXT("Same number of points"), EnginePoints.Num(), 2);
		TestEqual(TEXT("Point 0 moved to feet height"), EnginePoints[0].Location, FVector(0, 0, 10));
		TestTrue(TEXT("Mode 0 round-trips"), Atlantis::AI::DecodeTraversalModeFlags(EnginePoints[0].Flags) == EAtlantisTraversalMode::Swimming);
		TestTrue(TEXT("Mode 1 round-trips"), Atlantis::AI::DecodeTraversalModeFlags(EnginePoints[1].Flags) == EAtlantisTraversalMode::SurfaceWalking);
		TestTrue(TEXT("Nav-link bit (0x04) never set"), (EnginePoints[1].Flags & 0x04u) == 0u);
		TestTrue(TEXT("No node ref"), EnginePoints[0].NodeRef == INVALID_NAVNODEREF);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
