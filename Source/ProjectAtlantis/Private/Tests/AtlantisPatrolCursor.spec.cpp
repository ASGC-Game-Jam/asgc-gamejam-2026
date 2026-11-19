// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Patrol/AtlantisPatrolTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	/** Indices visited by advancing an unset cursor Steps times. */
	TArray<int32> Walk(int32 NumPoints, bool bClosedLoop, int32 Steps)
	{
		TArray<int32> Visited;
		FAtlantisPatrolCursor Cursor;
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Cursor = Atlantis::AI::AdvancePatrolCursor(Cursor, NumPoints, bClosedLoop);
			Visited.Add(Cursor.PointIndex);
		}
		return Visited;
	}
}

BEGIN_DEFINE_SPEC(FAtlantisPatrolCursorSpec, "Atlantis.AI.Patrol.Cursor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FAtlantisPatrolCursorSpec)

void FAtlantisPatrolCursorSpec::Define()
{
	It("wraps around on a closed route", [this]()
	{
		TestTrue(TEXT("Loop"), Walk(3, true, 7) == TArray<int32>{0, 1, 2, 0, 1, 2, 0});
	});

	It("ping-pongs between both ends of an open route", [this]()
	{
		TestTrue(TEXT("Ping-pong"), Walk(3, false, 8) == TArray<int32>{0, 1, 2, 1, 0, 1, 2, 1});
	});

	It("alternates on a two-point open route", [this]()
	{
		TestTrue(TEXT("Two points"), Walk(2, false, 5) == TArray<int32>{0, 1, 0, 1, 0});
	});

	It("stays on the only point of a one-point route", [this]()
	{
		TestTrue(TEXT("One point"), Walk(1, false, 3) == TArray<int32>{0, 0, 0});
		TestTrue(TEXT("One point, closed"), Walk(1, true, 3) == TArray<int32>{0, 0, 0});
	});

	It("returns an unset cursor for an empty route", [this]()
	{
		TestEqual(TEXT("Empty"), Atlantis::AI::AdvancePatrolCursor(FAtlantisPatrolCursor(), 0, false).PointIndex, static_cast<int32>(INDEX_NONE));
	});

	It("restarts from the first point when the cursor is out of range", [this]()
	{
		FAtlantisPatrolCursor Stale;
		Stale.PointIndex = 9;
		TestEqual(TEXT("Clamped"), Atlantis::AI::AdvancePatrolCursor(Stale, 3, false).PointIndex, 0);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
