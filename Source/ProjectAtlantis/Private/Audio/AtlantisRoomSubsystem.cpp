// Copyright (c) 2026 ASGC

#include "Audio/AtlantisRoomSubsystem.h"

#include "Audio/AtlantisAudioDataBPFL.h"
#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "StructUtils/InstancedStruct.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisRoom, Log, All);

void UAtlantisRoomSubsystem::EnterRoom(const UObject* Volume, FGameplayTag RoomTag, int32 Priority)
{
	ActiveRooms.RemoveAll([Volume](const FActiveRoom& Room) { return Room.Volume == Volume; });

	FActiveRoom& Room = ActiveRooms.AddDefaulted_GetRef();
	Room.Volume = Volume;
	Room.RoomTag = RoomTag;
	Room.Priority = Priority;

	UpdateCurrentRoom();
}

void UAtlantisRoomSubsystem::ExitRoom(const UObject* Volume)
{
	ActiveRooms.RemoveAll([Volume](const FActiveRoom& Room) { return Room.Volume == Volume; });
	UpdateCurrentRoom();
}

void UAtlantisRoomSubsystem::UpdateCurrentRoom()
{
	FGameplayTag Winner = TAG_Room_Default;
	int32 WinningPriority = MIN_int32;

	// >= so that on a tie, the most recently entered volume wins.
	for (const FActiveRoom& Room : ActiveRooms)
	{
		if (Room.Priority >= WinningPriority)
		{
			Winner = Room.RoomTag;
			WinningPriority = Room.Priority;
		}
	}

	// No volumes yet means Default, which the music already starts in, so there's nothing to send.
	const FGameplayTag PreviousRoom = CurrentRoom.IsValid() ? CurrentRoom : FGameplayTag(TAG_Room_Default);
	if (Winner == PreviousRoom)
	{
		return;
	}

	// A room missing from the table logs an error there, and is never sent as a guess.
	int32 RoomTypeId = 0;
	if (!UAtlantisAudioDeveloperSettings::GetRoomTypeId(Winner, RoomTypeId))
	{
		return;
	}

	CurrentRoom = Winner;
	UE_LOG(LogAtlantisRoom, Log, TEXT("Room changed from [%s] to [%s]."), *PreviousRoom.ToString(), *Winner.ToString());

	FAtlantisAudioTagPayload Payload;
	Payload.Tag = Winner;
	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Environment_Room, FInstancedStruct::Make(Payload));
}
