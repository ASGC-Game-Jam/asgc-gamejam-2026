// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AtlantisAudioFactBridgeComponent.generated.h"

class AController;
class APawn;
class UAtlantisHidingComponent;
class UAtlantisOxygenComponent;
class UAtlantisMovementComponent;

/**
 * Sends the local player's presence, oxygen, underwater and hiding to the audio ecosystem as facts.
 * On unbind it resets those facts, so listeners never stay stuck after death or unpossession.
 */
UCLASS(ClassGroup = (Atlantis), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisAudioFactBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Binds to oxygen and hiding once the owner is locally controlled, retrying for a few ticks until the player state arrives. Safe to call more than once. */
	void TryBindSources();

	/** Sends the reset facts, stops listening to every source and cancels any pending retry. Runs on end play and when the controller changes. */
	void UnbindSources();

	/** Sends each fact's current value once, so listeners are right from the start. */
	void SendInitialFacts();

	/** Sends available oxygen as a 0-1 fraction of capacity. */
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

	/** Sends entering water straight away, and leaving it only after Underwater Exit Delay. Bobbing at the surface sends nothing. */
	UFUNCTION()
	void HandleUnderwaterChanged(bool bUnderwater);

	/** Sends the underwater fact and remembers what listeners were told. */
	void SendUnderwater(bool bUnderwater);

	/** Called when the player has stayed out of the water for Underwater Exit Delay. */
	void HandleUnderwaterExitTimer();

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisOxygenComponent> OxygenComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisHidingComponent> HidingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisMovementComponent> MovementComponent;

	/** True while bound to a local player. Guards against sending reset facts twice. */
	bool bSourcesBound = false;

	/** Ticks spent waiting for an Atlantis player state. Reset whenever binding starts over. */
	int32 PlayerStateRetryCount = 0;

	/** Counts down Underwater Exit Delay after the player leaves the water. Cancelled if they go back in first. */
	FTimerHandle UnderwaterExitTimer;

	/** Seconds the player must stay out of the water before listeners hear they've left. Stops surface bobbing from flickering the fact. */
	UPROPERTY(EditAnywhere, Category = "Audio", meta = (ClampMin = "0.01"))
	float UnderwaterExitDelay = 0.5f;

	/** The underwater value listeners last heard. Stops repeat facts while the player bobs. */
	bool bUnderwaterSent = false;
};
