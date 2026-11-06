// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"

#include "AtlantisRoomSubsystem.generated.h"

/**
 * Works out which room the listener is in when room volumes overlap, and reports it once when it changes.
 * The highest priority volume wins. On a tie, the most recently entered one wins.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisRoomSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void EnterRoom(const UObject* Volume, FGameplayTag RoomTag, int32 Priority);
	void ExitRoom(const UObject* Volume);

	FGameplayTag GetCurrentRoom() const { return CurrentRoom; }

private:
	struct FActiveRoom
	{
		TWeakObjectPtr<const UObject> Volume;
		FGameplayTag RoomTag;
		int32 Priority = 0;
	};

	void UpdateCurrentRoom();

	TArray<FActiveRoom> ActiveRooms;
	FGameplayTag CurrentRoom;
};
