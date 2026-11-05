// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Interfaces/IAudioGameplayVolumeInteraction.h"

#include "AtlantisRoomVolumeComponent.generated.h"

/**
 * Add to an Audio Gameplay Volume to say which room it is.
 * Its priority comes from the Reverb Volume Component on the same volume, so the room and reverb always agree.
 * Don't disable this volume during play. If the listener is inside it, the music stays in this room.
 */
UCLASS(ClassGroup = (Atlantis), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisRoomVolumeComponent : public UActorComponent, public IAudioGameplayVolumeInteraction
{
	GENERATED_BODY()

public:
	/** The room this volume represents. */
	UPROPERTY(EditAnywhere, Category = "Atlantis Audio", meta = (Categories = "Room"))
	FGameplayTag RoomTag;

	virtual void OnListenerEnter_Implementation() override;
	virtual void OnListenerExit_Implementation() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
