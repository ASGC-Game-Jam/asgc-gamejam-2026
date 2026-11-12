// Copyright (c) 2026 ASGC

#include "AI/Patrol/AtlantisPatrolTypes.h"

FAtlantisPatrolCursor Atlantis::AI::AdvancePatrolCursor(const FAtlantisPatrolCursor& Cursor, int32 NumPoints, bool bClosedLoop)
{
	FAtlantisPatrolCursor Next;
	if (NumPoints <= 0)
	{
		return Next;
	}

	if (NumPoints == 1 || !(Cursor.PointIndex >= 0 && Cursor.PointIndex < NumPoints))
	{
		Next.PointIndex = 0;
		return Next;
	}

	if (bClosedLoop)
	{
		Next.PointIndex = (Cursor.PointIndex + 1) % NumPoints;
		return Next;
	}

	// Open route: walk to one end, then turn around.
	Next.Direction = Cursor.Direction;
	if (Next.Direction == EAtlantisPatrolDirection::Forward && Cursor.PointIndex >= NumPoints - 1)
	{
		Next.Direction = EAtlantisPatrolDirection::Reverse;
	}
	else if (Next.Direction == EAtlantisPatrolDirection::Reverse && Cursor.PointIndex <= 0)
	{
		Next.Direction = EAtlantisPatrolDirection::Forward;
	}

	Next.PointIndex = Cursor.PointIndex + (Next.Direction == EAtlantisPatrolDirection::Forward ? 1 : -1);
	return Next;
}
