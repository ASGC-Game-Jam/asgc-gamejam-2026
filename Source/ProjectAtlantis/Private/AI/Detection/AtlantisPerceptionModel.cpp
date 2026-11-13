// Copyright (c) 2026 ASGC

#include "AI/Detection/AtlantisPerceptionModel.h"

#include "Curves/CurveFloat.h"

namespace Atlantis::AI::Perception
{
	float GetSightDistance(const FVector& Viewer, const FVector& Target)
	{
		return FVector::Dist(Viewer, Target);
	}

	float ComputeSightFillRate(const FAtlantisDetectionFactors& Factors, const UCurveFloat* SightDistanceCurve)
	{
		if (SightDistanceCurve == nullptr)
		{
			return 0.f;
		}
		// Detection multipliers (light, movement, ...) would scale this.
		return FMath::Max(0.f, SightDistanceCurve->GetFloatValue(Factors.Distance));
	}

	void TickSensed(FAtlantisPlayerDetection& Data, float FillRate, float DeltaTime, const FVector& PlayerLocation, float Now)
	{
		Data.Meter = FMath::Min(1.f, Data.Meter + FillRate * DeltaTime);
		if (Data.Meter >= 1.f)
		{
			Data.bIsAggroed = true;
		}
		if (Data.bIsAggroed)
		{
			Data.Meter = 1.f;
		}

		Data.LastKnownLocation = PlayerLocation;
		Data.LastSensedTime = Now;
		Data.GraceTimeLeft.Reset();
	}

	void TickUnsensed(FAtlantisPlayerDetection& Data, float DeltaTime, const FAtlantisDetectionTuning& Tuning)
	{
		if (!Data.GraceTimeLeft.IsSet())
		{
			Data.GraceTimeLeft = Tuning.GraceTime;
		}

		float& Grace = Data.GraceTimeLeft.GetValue();
		Grace -= DeltaTime;
		if (Grace > 0.f)
		{
			return;
		}

		Grace = 0.f;
		Data.bIsAggroed = false;
		Data.Meter = FMath::Max(0.f, Data.Meter - Tuning.DecayPerSecond * DeltaTime);
	}

	void TickSuspicion(FAtlantisPlayerDetection& Data, bool bSensed, float DeltaTime, const FAtlantisDetectionTuning& Tuning)
	{
		Data.Meter = FMath::Clamp(Data.Meter, 0.f, 1.f);

		if (bSensed)
		{
			// Spotted again while still searching: no need to build suspicion from scratch.
			if (Data.SearchTimeLeft > 0.f)
			{
				Data.Meter = FMath::Max(Data.Meter, Tuning.SuspicionThreshold);
			}
		}
		else if (Data.bWasSensedLastTick && (Data.bIsAggroed || Data.Meter >= Tuning.SuspicionThreshold))
		{
			// The moment we lose a suspicious player: start (or restart) the search clock.
			Data.SearchTimeLeft = Tuning.MaxSearchDuration;
		}
		else
		{
			Data.SearchTimeLeft = FMath::Max(0.f, Data.SearchTimeLeft - DeltaTime);
		}

		Data.bWasSensedLastTick = bSensed;
	}

	bool ShouldForget(const FAtlantisPlayerDetection& Data)
	{
		return Data.Meter <= 0.f && Data.SearchTimeLeft <= 0.f && !Data.bIsAggroed;
	}

	int32 GetThreatRank(const FAtlantisPlayerDetection& Data, const FAtlantisDetectionTuning& Tuning)
	{
		if (Data.bIsAggroed)
		{
			return 3;
		}
		const bool bSuspicious = Data.Meter >= Tuning.SuspicionThreshold;
		if (Data.bWasSensedLastTick)
		{
			return bSuspicious ? 2 : 0;
		}
		return (bSuspicious || Data.SearchTimeLeft > 0.f) ? 1 : 0;
	}

	EMonsterStatus ComputeMonsterStatus(TConstArrayView<const FAtlantisPlayerDetection*> Entries, const FAtlantisDetectionTuning& Tuning)
	{
		int32 BestRank = 0;
		for (const FAtlantisPlayerDetection* Entry : Entries)
		{
			if (Entry != nullptr)
			{
				BestRank = FMath::Max(BestRank, GetThreatRank(*Entry, Tuning));
			}
		}

		switch (BestRank)
		{
		case 3:  return EMonsterStatus::Combat;
		case 2:  return EMonsterStatus::Alerted;
		case 1:  return EMonsterStatus::Searching;
		default: return EMonsterStatus::Idle;
		}
	}

	int32 FindSearchTarget(TConstArrayView<const FAtlantisPlayerDetection*> Entries, const FAtlantisDetectionTuning& Tuning)
	{
		int32 BestIndex = INDEX_NONE;
		for (int32 Index = 0; Index < Entries.Num(); ++Index)
		{
			const FAtlantisPlayerDetection* Entry = Entries[Index];
			if (Entry == nullptr)
			{
				continue;
			}
			if (BestIndex == INDEX_NONE)
			{
				BestIndex = Index;
				continue;
			}

			const FAtlantisPlayerDetection& Best = *Entries[BestIndex];
			const int32 Rank = GetThreatRank(*Entry, Tuning);
			const int32 BestRank = GetThreatRank(Best, Tuning);
			if (Rank != BestRank)
			{
				if (Rank > BestRank)
				{
					BestIndex = Index;
				}
				continue;
			}
			if (!FMath::IsNearlyEqual(Entry->Meter, Best.Meter))
			{
				if (Entry->Meter > Best.Meter)
				{
					BestIndex = Index;
				}
				continue;
			}
			if (Entry->LastSensedTime > Best.LastSensedTime)
			{
				BestIndex = Index;
			}
		}
		return BestIndex;
	}
}
