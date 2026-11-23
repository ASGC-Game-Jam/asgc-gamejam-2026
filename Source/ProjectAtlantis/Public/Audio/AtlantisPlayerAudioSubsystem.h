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
 * Plays the player's own suit sounds, such as breathing and oxygen alarms, from audio facts.
 * Starts its loops when Audio.Fact.Player.Present is true, and stops them all when it's false.
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

	/** Stops every loop. */
	virtual void Deinitialize() override;

	/** Starts, stops or updates the suit sound loops from a fact. */
	virtual void OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload) override;

protected:
	/** Only game and PIE worlds, because editor viewport worlds have no player. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Starts every Loop row's sound that isn't already playing. */
	void RefreshPlayerLoops();

	/** Stops every playing loop and forgets it. Safe to call when nothing is playing. */
	void StopPlayerLoops();

	bool ShouldLoopPlay(const FAtlantisPlayerAudioRow& Row) const;

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

	TMap<FGameplayTag, bool> BoolFacts;

	TMap<FGameplayTag, float> FloatFacts;

};
