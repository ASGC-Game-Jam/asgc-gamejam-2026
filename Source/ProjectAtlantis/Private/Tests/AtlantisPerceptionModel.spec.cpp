// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/AtlantisPerceptionModel.h"
#include "Curves/CurveFloat.h"
#include "Misc/AutomationTest.h"

using namespace Atlantis::AI::Perception;

namespace
{
	constexpr float Dt = 0.1f;

	FAtlantisDetectionTuning MakeTestTuning()
	{
		FAtlantisDetectionTuning Tuning;
		Tuning.SuspicionThreshold = 0.3f;
		Tuning.MaxSearchDuration = 45.f;
		Tuning.GraceTime = 5.f;
		Tuning.DecayPerSecond = 0.1f;
		return Tuning;
	}

	/** One full tick for a single player, like UAtlantisDetectionComponent does. */
	void Step(FAtlantisPlayerDetection& Data, bool bSensed, float FillRate, const FAtlantisDetectionTuning& Tuning, float& Now, float DeltaTime = Dt)
	{
		Now += DeltaTime;
		if (bSensed)
		{
			TickSensed(Data, FillRate, DeltaTime, FVector(Now, 0.f, 0.f), Now);
		}
		else
		{
			TickUnsensed(Data, DeltaTime, Tuning);
		}
		TickSuspicion(Data, bSensed, DeltaTime, Tuning);
	}

	void StepFor(FAtlantisPlayerDetection& Data, bool bSensed, float FillRate, const FAtlantisDetectionTuning& Tuning, float& Now, float Seconds)
	{
		for (float Elapsed = 0.f; Elapsed < Seconds - KINDA_SMALL_NUMBER; Elapsed += Dt)
		{
			Step(Data, bSensed, FillRate, Tuning, Now);
		}
	}

	EMonsterStatus StatusOf(const FAtlantisPlayerDetection& Data, const FAtlantisDetectionTuning& Tuning)
	{
		const FAtlantisPlayerDetection* Entries[] = { &Data };
		return ComputeMonsterStatus(Entries, Tuning);
	}

	/** Same as UAtlantisDetectionComponent::RefreshStatus, over a whole player map. */
	TArray<const FAtlantisPlayerDetection*> EntriesOf(const TMap<int32, FAtlantisPlayerDetection>& Players)
	{
		TArray<const FAtlantisPlayerDetection*> Entries;
		for (const TPair<int32, FAtlantisPlayerDetection>& Pair : Players)
		{
			Entries.Add(&Pair.Value);
		}
		return Entries;
	}
}

BEGIN_DEFINE_SPEC(FAtlantisPerceptionModelSpec, "Atlantis.AI.Perception.Model",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FAtlantisDetectionTuning Tuning;
	float Now = 0.f;
END_DEFINE_SPEC(FAtlantisPerceptionModelSpec)

void FAtlantisPerceptionModelSpec::Define()
{
	BeforeEach([this]()
	{
		Tuning = MakeTestTuning();
		Now = 0.f;
	});

	It("measures sight distance in 3D (height counts)", [this]()
	{
		TestEqual(TEXT("Straight up"), GetSightDistance(FVector::ZeroVector, FVector(0.f, 0.f, 500.f)), 500.f);
		TestEqual(TEXT("Diagonal"), GetSightDistance(FVector::ZeroVector, FVector(300.f, 0.f, 400.f)), 500.f);
	});

	It("reads the fill rate from the distance curve, never negative, 0 without a curve", [this]()
	{
		UCurveFloat* Curve = NewObject<UCurveFloat>();
		Curve->FloatCurve.AddKey(0.f, 0.5f);
		Curve->FloatCurve.AddKey(1000.f, -1.f);
		FAtlantisDetectionFactors Factors;
		Factors.Distance = 0.f;
		TestEqual(TEXT("Close"), ComputeSightFillRate(Factors, Curve), 0.5f);
		Factors.Distance = 1000.f;
		TestEqual(TEXT("Clamped"), ComputeSightFillRate(Factors, Curve), 0.f);
		TestEqual(TEXT("No curve"), ComputeSightFillRate(Factors, nullptr), 0.f);
	});

	It("never detects instantly, even at point-blank range", [this]()
	{
		FAtlantisPlayerDetection Data;
		Step(Data, true, 0.5f, Tuning, Now);
		TestTrue(TEXT("Not aggroed after one tick"), !Data.bIsAggroed);
		TestTrue(TEXT("Meter small"), Data.Meter < 0.1f);
	});

	It("aggroes only when the meter is full", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 1.9f);
		TestFalse(TEXT("Not yet at 1.9 s"), Data.bIsAggroed);
		StepFor(Data, true, 0.5f, Tuning, Now, 0.2f);
		TestTrue(TEXT("Aggroed at 2 s"), Data.bIsAggroed);
		TestEqual(TEXT("Clamped to 1"), Data.Meter, 1.f);
		TestTrue(TEXT("Combat"), StatusOf(Data, Tuning) == EMonsterStatus::Combat);
	});

	It("stays Idle while seen below the threshold, Alerted at or above it", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 0.4f);
		TestTrue(TEXT("Glimpse = Idle"), StatusOf(Data, Tuning) == EMonsterStatus::Idle);
		StepFor(Data, true, 0.5f, Tuning, Now, 0.3f);
		TestTrue(TEXT("Suspicious = Alerted"), StatusOf(Data, Tuning) == EMonsterStatus::Alerted);
	});

	It("forgets a brief glimpse without searching", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 0.2f);
		Step(Data, false, 0.f, Tuning, Now);
		TestEqual(TEXT("No search armed"), Data.SearchTimeLeft, 0.f);
		TestTrue(TEXT("Idle"), StatusOf(Data, Tuning) == EMonsterStatus::Idle);
	});

	It("searches after losing a suspicious player, for at most MaxSearchDuration", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 1.f);
		Step(Data, false, 0.f, Tuning, Now);
		TestEqual(TEXT("Search armed"), Data.SearchTimeLeft, Tuning.MaxSearchDuration);
		TestTrue(TEXT("Searching"), StatusOf(Data, Tuning) == EMonsterStatus::Searching);
		StepFor(Data, false, 0.f, Tuning, Now, Tuning.MaxSearchDuration + 1.f);
		TestEqual(TEXT("Search expired"), Data.SearchTimeLeft, 0.f);
		TestTrue(TEXT("Idle after the cap"), StatusOf(Data, Tuning) == EMonsterStatus::Idle);
		TestTrue(TEXT("Forgettable"), ShouldForget(Data));
	});

	It("counts the search down only while the player is unseen", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 1.f);
		StepFor(Data, false, 0.f, Tuning, Now, 2.f);
		const float Left = Data.SearchTimeLeft;
		Step(Data, true, 0.f, Tuning, Now);
		TestEqual(TEXT("Paused while seen"), Data.SearchTimeLeft, Left);
	});

	It("keeps Combat during the grace time, then searches", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 2.1f);
		StepFor(Data, false, 0.f, Tuning, Now, 4.f);
		TestTrue(TEXT("Still Combat in grace"), StatusOf(Data, Tuning) == EMonsterStatus::Combat);
		StepFor(Data, false, 0.f, Tuning, Now, 1.5f);
		TestFalse(TEXT("Aggro ended"), Data.bIsAggroed);
		TestTrue(TEXT("Searching"), StatusOf(Data, Tuning) == EMonsterStatus::Searching);
		TestTrue(TEXT("Meter draining"), Data.Meter < 1.f);
	});

	It("goes straight back to Alerted when the player is seen again mid-search", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 0.8f);
		StepFor(Data, false, 0.f, Tuning, Now, 20.f);
		TestTrue(TEXT("Meter drained below threshold"), Data.Meter < Tuning.SuspicionThreshold);
		Step(Data, true, 0.f, Tuning, Now);
		TestTrue(TEXT("Meter lifted"), Data.Meter >= Tuning.SuspicionThreshold);
		TestTrue(TEXT("Alerted"), StatusOf(Data, Tuning) == EMonsterStatus::Alerted);
	});

	It("ranks players: aggroed > seen & suspicious > lost & searched for, then meter, then most recent", [this]()
	{
		FAtlantisPlayerDetection Searching;
		Searching.Meter = 0.9f;
		Searching.SearchTimeLeft = 10.f;
		FAtlantisPlayerDetection Seen;
		Seen.Meter = 0.4f;
		Seen.bWasSensedLastTick = true;
		FAtlantisPlayerDetection Aggro;
		Aggro.Meter = 1.f;
		Aggro.bIsAggroed = true;

		const FAtlantisPlayerDetection* Two[] = { &Searching, &Seen };
		TestEqual(TEXT("Seen beats searched-for"), FindSearchTarget(Two, Tuning), 1);
		const FAtlantisPlayerDetection* Three[] = { &Searching, &Seen, &Aggro };
		TestEqual(TEXT("Aggro beats all"), FindSearchTarget(Three, Tuning), 2);

		FAtlantisPlayerDetection Older = Seen;
		Older.LastSensedTime = 1.f;
		FAtlantisPlayerDetection Newer = Seen;
		Newer.LastSensedTime = 2.f;
		const FAtlantisPlayerDetection* Tie[] = { &Older, &Newer };
		TestEqual(TEXT("Tie -> most recent"), FindSearchTarget(Tie, Tuning), 1);
		TestEqual(TEXT("Empty"), FindSearchTarget(TConstArrayView<const FAtlantisPlayerDetection*>(), Tuning), static_cast<int32>(INDEX_NONE));
	});

	It("picks the status of the most important player", [this]()
	{
		FAtlantisPlayerDetection Glimpse;
		Glimpse.Meter = 0.1f;
		Glimpse.bWasSensedLastTick = true;
		FAtlantisPlayerDetection Lost;
		Lost.Meter = 0.5f;
		Lost.SearchTimeLeft = 5.f;
		const FAtlantisPlayerDetection* Entries[] = { &Glimpse, &Lost };
		TestTrue(TEXT("Searching wins over a glimpse"), ComputeMonsterStatus(Entries, Tuning) == EMonsterStatus::Searching);
		TestTrue(TEXT("Nobody = Idle"), ComputeMonsterStatus(TConstArrayView<const FAtlantisPlayerDetection*>(), Tuning) == EMonsterStatus::Idle);
	});

	It("forgets only empty, non-searched, non-aggroed players", [this]()
	{
		FAtlantisPlayerDetection Data;
		TestTrue(TEXT("Empty"), ShouldForget(Data));
		Data.SearchTimeLeft = 1.f;
		TestFalse(TEXT("Searching"), ShouldForget(Data));
		Data.SearchTimeLeft = 0.f;
		Data.Meter = 0.01f;
		TestFalse(TEXT("Meter left"), ShouldForget(Data));
	});

	It("does not re-arm the search after it expired", [this]()
	{
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 1.f);
		Step(Data, false, 0.f, Tuning, Now);
		TestEqual(TEXT("Search armed"), Data.SearchTimeLeft, Tuning.MaxSearchDuration);
		StepFor(Data, false, 0.f, Tuning, Now, Tuning.MaxSearchDuration + 1.f);
		TestEqual(TEXT("Search expired"), Data.SearchTimeLeft, 0.f);

		float MaxSearchLeft = 0.f;
		for (int32 StepIndex = 0; StepIndex < 100; ++StepIndex)
		{
			Step(Data, false, 0.f, Tuning, Now);
			MaxSearchLeft = FMath::Max(MaxSearchLeft, Data.SearchTimeLeft);
		}
		TestEqual(TEXT("Stays expired while unseen"), MaxSearchLeft, 0.f);
		TestTrue(TEXT("Meter below threshold"), Data.Meter < Tuning.SuspicionThreshold);
		TestTrue(TEXT("Idle"), StatusOf(Data, Tuning) == EMonsterStatus::Idle);
	});

	It("treats a concealed player like an unseen one", [this]()
	{
		// The component skips concealed (hiding) players before TickSensed, so hiding = a tick with bSensed false.
		FAtlantisPlayerDetection Data;
		StepFor(Data, true, 0.5f, Tuning, Now, 1.f);
		const FVector SeenAt = Data.LastKnownLocation;
		TestTrue(TEXT("Alerted before hiding"), StatusOf(Data, Tuning) == EMonsterStatus::Alerted);

		Step(Data, false, 0.f, Tuning, Now);
		TestEqual(TEXT("Search armed on the hiding edge"), Data.SearchTimeLeft, Tuning.MaxSearchDuration);
		TestEqual(TEXT("Last known location is where they hid"), Data.LastKnownLocation, SeenAt);
		TestTrue(TEXT("Searching"), StatusOf(Data, Tuning) == EMonsterStatus::Searching);
		TestTrue(TEXT("Grace running"), Data.GraceTimeLeft.IsSet());
	});

	It("goes Idle after giving up the search (ForgetUnsensed), keeping players still in sight", [this]()
	{
		TMap<int32, FAtlantisPlayerDetection> Players;
		FAtlantisPlayerDetection& Lost = Players.Add(0);
		StepFor(Lost, true, 0.5f, Tuning, Now, 1.f);
		StepFor(Lost, false, 0.f, Tuning, Now, 2.f);
		FAtlantisPlayerDetection& Glimpse = Players.Add(1);
		Glimpse.Meter = 0.1f;
		Glimpse.bWasSensedLastTick = true;

		TestTrue(TEXT("Searching before giving up"), ComputeMonsterStatus(EntriesOf(Players), Tuning) == EMonsterStatus::Searching);
		ForgetUnsensed(Players);
		TestEqual(TEXT("Only the player in sight is kept"), Players.Num(), 1);
		TestTrue(TEXT("Glimpse kept"), Players.Contains(1));
		TestTrue(TEXT("Idle next status computation"), ComputeMonsterStatus(EntriesOf(Players), Tuning) == EMonsterStatus::Idle);

		ForgetUnsensed(Players);
		TestEqual(TEXT("Giving up twice changes nothing"), Players.Num(), 1);
	});

	It("prunes forgotten players before the status and suspect are computed", [this]()
	{
		TMap<int32, FAtlantisPlayerDetection> Players;
		FAtlantisPlayerDetection& Expired = Players.Add(0);
		StepFor(Expired, true, 0.5f, Tuning, Now, 1.f);
		StepFor(Expired, false, 0.f, Tuning, Now, Tuning.MaxSearchDuration + 1.f);

		// Same order as UAtlantisDetectionComponent::UpdateDetection: prune, then RefreshStatus.
		for (auto It = Players.CreateIterator(); It; ++It)
		{
			if (ShouldForget(It.Value()))
			{
				It.RemoveCurrent();
			}
		}
		TestEqual(TEXT("Expired player pruned"), Players.Num(), 0);
		const TArray<const FAtlantisPlayerDetection*> Entries = EntriesOf(Players);
		TestEqual(TEXT("No suspect left"), FindSearchTarget(Entries, Tuning), static_cast<int32>(INDEX_NONE));
		TestTrue(TEXT("Idle"), ComputeMonsterStatus(Entries, Tuning) == EMonsterStatus::Idle);
	});
}

#endif // WITH_DEV_AUTOMATION_TESTS
