// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AI/AtlantisAITypes.h"

class UCurveFloat;

/** What an enemy remembers about one player. Owned by UAtlantisDetectionComponent. */
struct FAtlantisPlayerDetection
{
	/** Detection meter, 0-1. 1 = spotted (aggro). */
	float Meter = 0.f;
	/** Where the player was last seen. */
	FVector LastKnownLocation = FVector::ZeroVector;
	/** Grace time left after losing sight before the meter drains (and aggro ends). Unset while seen. */
	TOptional<float> GraceTimeLeft;
	bool bIsAggroed = false;
	/** Time left to keep searching for this player after losing them. Counts down only while unseen. */
	float SearchTimeLeft = 0.f;
	/** Seen on the latest tick (after TickSuspicion: this tick; used to detect the moment sight is lost). */
	bool bWasSensedLastTick = false;
	/** World time the player was last seen. */
	float LastSensedTime = -1.f;
};

/** Detection tuning, set on UAtlantisDetectionComponent. */
struct FAtlantisDetectionTuning
{
	/** Meter level at which the enemy reacts (Alerted while seeing the player, Searching after losing them). */
	float SuspicionThreshold = 0.3f;
	/** Max time an enemy keeps searching after losing a suspicious player. */
	float MaxSearchDuration = 45.f;
	/** Grace time after losing sight before the meter drains (and aggro ends). */
	float GraceTime = 5.f;
	/** How fast the meter drains once the grace time is over, per second. */
	float DecayPerSecond = 0.1f;
};

/**
 * Everything that changes how fast a player is detected. Only distance today; light, movement, noise etc. become
 * extra fields here (seam S5) without touching the callers.
 */
struct FAtlantisDetectionFactors
{
	/** 3D distance between enemy and player, in cm. */
	float Distance = 0.f;
};

/** Memory for one search: where it started and which sweep points were already checked. */
struct FAtlantisSearchMemory
{
	void BeginSearch(const FVector& InOrigin)
	{
		Origin = InOrigin;
		Visited.Reset();
	}

	void MarkVisited(const FVector& Point) { Visited.Add(Point); }
	int32 GetSweepsDone() const { return Visited.Num(); }

	FVector Origin = FVector::ZeroVector;
	TArray<FVector> Visited;
};

/** Pure detection rules, shared by UAtlantisDetectionComponent and its automation spec. */
namespace Atlantis::AI::Perception
{
	/** Distance used for detection: full 3D (a player above or below counts as far away). */
	PROJECTATLANTIS_API float GetSightDistance(const FVector& Viewer, const FVector& Target);

	/** Meter gained per second while seen. Reads the distance curve; 0 without a curve. Never negative. */
	PROJECTATLANTIS_API float ComputeSightFillRate(const FAtlantisDetectionFactors& Factors, const UCurveFloat* SightDistanceCurve);

	/** The player was seen this tick: fill the meter, refresh last known location, cancel grace. Aggro at 1. */
	PROJECTATLANTIS_API void TickSensed(FAtlantisPlayerDetection& Data, float FillRate, float DeltaTime, const FVector& PlayerLocation, float Now);

	/** The player wasn't seen this tick: run the grace time, then drain the meter. Aggro ends with the grace time. */
	PROJECTATLANTIS_API void TickUnsensed(FAtlantisPlayerDetection& Data, float DeltaTime, const FAtlantisDetectionTuning& Tuning);

	/**
	 * Search bookkeeping, after TickSensed/TickUnsensed. Losing sight of a suspicious (>= threshold) or aggroed
	 * player arms SearchTimeLeft; it counts down only while unseen. Seeing the player again mid-search lifts the
	 * meter to the threshold (straight back to Alerted).
	 */
	PROJECTATLANTIS_API void TickSuspicion(FAtlantisPlayerDetection& Data, bool bSensed, float DeltaTime, const FAtlantisDetectionTuning& Tuning);

	/** Nothing left to remember: empty meter, not aggroed, no search running. */
	PROJECTATLANTIS_API bool ShouldForget(const FAtlantisPlayerDetection& Data);

	/** How much this player matters: 3 aggroed, 2 seen & suspicious, 1 lost & still searched for, 0 otherwise. */
	PROJECTATLANTIS_API int32 GetThreatRank(const FAtlantisPlayerDetection& Data, const FAtlantisDetectionTuning& Tuning);

	/** Combat (anyone aggroed) > Alerted (seen & suspicious) > Searching (lost & still searched for) > Idle. */
	PROJECTATLANTIS_API EMonsterStatus ComputeMonsterStatus(TConstArrayView<const FAtlantisPlayerDetection*> Entries, const FAtlantisDetectionTuning& Tuning);

	/** Index of the player to react to: highest rank, then fullest meter, then most recently seen. INDEX_NONE if empty. */
	PROJECTATLANTIS_API int32 FindSearchTarget(TConstArrayView<const FAtlantisPlayerDetection*> Entries, const FAtlantisDetectionTuning& Tuning);

	/** Giving up the search: drops every player not seen on the latest tick (meter, aggro and search forgotten). */
	template <typename KeyType>
	void ForgetUnsensed(TMap<KeyType, FAtlantisPlayerDetection>& Players)
	{
		for (auto It = Players.CreateIterator(); It; ++It)
		{
			if (!It.Value().bWasSensedLastTick)
			{
				It.RemoveCurrent();
			}
		}
	}
}
