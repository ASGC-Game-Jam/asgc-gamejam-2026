// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AtlantisAudioFactBridgeComponent.generated.h"

class AController;
class APawn;
class UAtlantisHidingComponent;
class UAtlantisOxygenComponent;

/**
 * Forwards the local player's oxygen and hiding events to the audio ecosystem as facts.
 * Only translates events into facts. How a fact sounds is decided by the music axis mapping.
 * Investigations aren't per-pawn, so they're reported by UAtlantisJournalAudioBridge instead.
 */
UCLASS(ClassGroup = (Atlantis), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisAudioFactBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Binds to the sources once the owner is locally controlled. Waits a limited number of ticks for the player state. Safe to call more than once. */
	void TryBindSources();

	/** Stops listening to every source and cancels any pending retry. Called on end play, and before rebinding when the controller changes. */
	void UnbindSources();

	/** Sends each fact's current value once, so the music is right from the start. */
	void SendInitialFacts();

	void SendOxygenLevel();

	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	UFUNCTION()
	void HandleCurrentOxygenChanged(float OldCurrentOxygen, float NewCurrentOxygen);

	UFUNCTION()
	void HandleCriticalOxygenChanged(bool bCriticalOxygen);

	UFUNCTION()
	void HandleAllocatedOxygenChanged(float OldAllocatedOxygen, float NewAllocatedOxygen);

	UFUNCTION()
	void HandleOxygenCapacityChanged(float OldOxygenCapacity, float NewOxygenCapacity);

	UFUNCTION()
	void HandleHidingStateChanged(bool bHiding);

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisOxygenComponent> OxygenComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisHidingComponent> HidingComponent;

	bool bSourcesBound = false;

	/** Ticks spent waiting for an Atlantis player state. Reset whenever binding starts over. */
	int32 PlayerStateRetryCount = 0;

};
