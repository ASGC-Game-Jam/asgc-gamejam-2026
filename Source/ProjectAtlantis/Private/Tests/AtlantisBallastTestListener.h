// Copyright (c) 2026 ASGC

#pragma once

#include "Core/AtlantisPlayerState.h"
#include "AtlantisBallastTestListener.generated.h"

/** Records the real dynamic delegates emitted by player state during automation tests. */
UCLASS(Transient)
class UAtlantisBallastTestListener : public UObject
{
	GENERATED_BODY()

public:
	int32 BallastEventCount = 0;
	int32 LockedOxygenEventCount = 0;
	bool bLastAllocated = false;
	EAtlantisBallastState LastBallastState = EAtlantisBallastState::None;
	float LastOldLockedOxygen = 0.f;
	float LastNewLockedOxygen = 0.f;

	void Reset()
	{
		BallastEventCount = 0;
		LockedOxygenEventCount = 0;
		bLastAllocated = false;
		LastBallastState = EAtlantisBallastState::None;
		LastOldLockedOxygen = 0.f;
		LastNewLockedOxygen = 0.f;
	}

	UFUNCTION()
	void RecordBallastChanged(bool bIsAllocated, EAtlantisBallastState BallastState)
	{
		++BallastEventCount;
		bLastAllocated = bIsAllocated;
		LastBallastState = BallastState;
	}

	UFUNCTION()
	void RecordLockedOxygenChanged(float OldLockedOxygen, float NewLockedOxygen)
	{
		++LockedOxygenEventCount;
		LastOldLockedOxygen = OldLockedOxygen;
		LastNewLockedOxygen = NewLockedOxygen;
	}
};
