// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"

#include "AtlantisPlayerAudioSubsystem.generated.h"

class UDataTable;
class USoundBase;
class UAudioComponent;
struct FAtlantisPlayerAudioRow;

/**
 * Plays the player's own suit sounds, such as breathing and oxygen alarms, from the rows in DT_PlayerAudio.
 * Each Loop row plays while the player is present and its Play While Fact is true, and only underwater if Underwater Only is ticked.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisPlayerAudioSubsystem : public UWorldSubsystem, public IASGC_FactListenerInterface
{
	GENERATED_BODY()

public:
	/** Subscribes to the player facts. Runs before any actor's BeginPlay, so no fact is missed. */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Unsubscribes from the player facts. Runs before any subsystem deinitializes, so the router still exists. */
	virtual void PreDeinitialize() override;

	/** Stops every loop and forgets the stored facts. */
	virtual void Deinitialize() override;

	/** Stores the fact. A bool fact rechecks which loops should play, and a float fact updates the loops that use it as a parameter. */
	virtual void OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload) override;

protected:
	/** Only game and PIE worlds, because editor viewport worlds have no player. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Starts each Loop row that should play and isn't, and fades out each one that's playing and shouldn't be. */
	void RefreshPlayerLoops();

	/** Stops every playing loop at once, without a fade, and forgets it. Safe to call when nothing is playing. */
	void StopPlayerLoops();

	/** True if the player is present, the row's Play While Fact is true, and the player is underwater when the row needs it. */
	bool ShouldLoopPlay(const FAtlantisPlayerAudioRow& Row) const;

	/** Sends the row's Parameter Fact value to its MetaSound input. Does nothing if the row has no parameter, the fact hasn't arrived, or the component is gone. */
	void ApplyParameter(const FAtlantisPlayerAudioRow& Row, UAudioComponent* AudioComp) const;

private:
	/** Each fact tag this subsystem subscribed to, and its router handle for unsubscribing. */
	TMap<FGameplayTag, int32> FactSubscriptions;

	/** The player audio table from Project Settings, loaded on world begin play. Null if none is set. */
	UPROPERTY()
	TObjectPtr<UDataTable> PlayerAudio = nullptr;

	/** Each looping sound that's playing, and the audio component playing it. One component per sound, even when rows share it. */
	UPROPERTY()
	TMap<TObjectPtr<USoundBase>, TObjectPtr<UAudioComponent>> PlayingPlayerAudio;

	/** The last value of each bool fact received. The router doesn't remember facts, so this subsystem does. */
	TMap<FGameplayTag, bool> BoolFacts;

	/** The last value of each float fact received, so a loop that starts later gets the current value. */
	TMap<FGameplayTag, float> FloatFacts;
};
