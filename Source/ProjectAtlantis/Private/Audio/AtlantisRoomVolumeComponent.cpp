// Copyright (c) 2026 ASGC

#include "Audio/AtlantisRoomVolumeComponent.h"

#include "Audio/AtlantisRoomSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "ReverbVolumeComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisRoomVolume, Log, All);

void UAtlantisRoomVolumeComponent::OnListenerEnter_Implementation()
{
	if (!RoomTag.IsValid())
	{
		UE_LOG(LogAtlantisRoomVolume, Error, TEXT("Room volume [%s] has no Room Tag. Pick one in the Details panel."), *GetNameSafe(GetOwner()));
		return;
	}

	const UReverbVolumeComponent* Reverb = GetOwner()->FindComponentByClass<UReverbVolumeComponent>();
	const int32 Priority = Reverb ? Reverb->GetPriority() : 0;

	if (UAtlantisRoomSubsystem* Rooms = GetWorld()->GetSubsystem<UAtlantisRoomSubsystem>())
	{
		Rooms->EnterRoom(this, RoomTag, Priority);
	}
}

void UAtlantisRoomVolumeComponent::OnListenerExit_Implementation()
{
	if (UAtlantisRoomSubsystem* Rooms = GetWorld()->GetSubsystem<UAtlantisRoomSubsystem>())
	{
		Rooms->ExitRoom(this);
	}
}

void UAtlantisRoomVolumeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// If this volume unloads while the listener is inside it, the room still needs to change.
	if (const UWorld* World = GetWorld())
	{
		if (UAtlantisRoomSubsystem* Rooms = World->GetSubsystem<UAtlantisRoomSubsystem>())
		{
			Rooms->ExitRoom(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}
