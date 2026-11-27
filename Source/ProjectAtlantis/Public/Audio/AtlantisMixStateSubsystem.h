// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"

#include "AtlantisMixStateSubsystem.generated.h"

class USoundControlBusMix;

/**
 * Turns control bus mixes on and off from audio facts, using the Mix States list in Project Settings.
 * Each rule's mix is on while its bool fact is true, such as CBM_Underwater while the player is underwater.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisMixStateSubsystem : public UWorldSubsystem, public IASGC_FactListenerInterface
{
	GENERATED_BODY()

public:
	/** Subscribes to each rule's fact. Runs before any actor's BeginPlay, so no fact is missed. */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Unsubscribes, and turns off every mix that's on. Mixes live on the audio device, so they'd outlive the level otherwise. */
	virtual void PreDeinitialize() override;

	/** Turns each matching rule's mix on when a bool fact is true, and off when it's false. */
	virtual void OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload) override;

protected:
	/** Only game and PIE worlds, because editor viewport worlds have no player. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Each fact tag this subsystem subscribed to, and its router handle for unsubscribing. */
	TMap<FGameplayTag, int32> FactSubscriptions;

	/** Every mix this subsystem has turned on, so they can all be turned off at level end. */
	UPROPERTY()
	TSet<TObjectPtr<USoundControlBusMix>> ActiveMixes;
};
