// Copyright (c) 2026 ASGC

#include "AI/AtlantisSearchPointSource.h"

#include "AI/AtlantisNavGraphSubsystem.h"
#include "Engine/World.h"

bool FAtlantisSearchPointSource_NavGraph::FindPoint(const FAtlantisSearchPointQuery& Query, FVector& OutPoint) const
{
	UAtlantisNavGraphSubsystem* NavGraph = Query.World != nullptr ? Query.World->GetSubsystem<UAtlantisNavGraphSubsystem>() : nullptr;
	if (NavGraph == nullptr)
	{
		return false;
	}

	TArray<FVector> Candidates;
	NavGraph->GetReachableNodeLocations(Query.From, Query.Origin, Radius, FAtlantisNavQueryFilter(), Candidates);

	const float MinDistSq = FMath::Square(MinDistanceFromOrigin);
	const float VisitedSq = FMath::Square(VisitedTolerance);
	Candidates.RemoveAll([&](const FVector& Candidate)
	{
		if (FVector::DistSquared(Candidate, Query.Origin) < MinDistSq)
		{
			return true;
		}
		for (const FVector& Visited : Query.Visited)
		{
			if (FVector::DistSquared(Candidate, Visited) < VisitedSq)
			{
				return true;
			}
		}
		return false;
	});

	if (Candidates.IsEmpty())
	{
		return false;
	}
	OutPoint = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	return true;
}
