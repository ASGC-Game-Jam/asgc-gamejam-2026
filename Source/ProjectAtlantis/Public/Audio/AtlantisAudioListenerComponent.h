// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisAudioListenerComponent.generated.h"

class APawn;
class AController;

/**
 * Measures sound distance from a point between the camera and the character, so nearby sounds aren't heard from the camera 400 units back.
 * Panning still comes from the camera, so left and right match the screen.
 */
UCLASS(ClassGroup=(Atlantis), meta=(BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisAudioListenerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Ticks after the camera has moved, so the listener point matches this frame's view. */
	UAtlantisAudioListenerComponent();

protected:
	/** Caches the owning pawn and watches for controller changes. Stops ticking if the owner isn't a pawn. */
	virtual void BeginPlay() override;

	/** Clears the override, so the controller doesn't keep a listener point for a pawn that's gone. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** Moves the listener's attenuation point to Listener Blend of the way from the camera to the character. */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Clears the override on the old controller. The new one gets its point on the next tick. */
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	/** Clears the attenuation override if Controller is a player controller. */
	void ClearOverride(AController* Controller);

private:
	/** Where distance is measured from: 0 is the camera, 1 is the character's eyes. */
	UPROPERTY(EditAnywhere, Category="Audio", meta=(ClampMin="0", ClampMax="1"))
	float ListenerBlend = 0.5f;

	/** The pawn this component is on. A component's owner never changes, so it's cached once. */
	UPROPERTY(Transient)
	TObjectPtr<APawn> OwnerPawn;
};
