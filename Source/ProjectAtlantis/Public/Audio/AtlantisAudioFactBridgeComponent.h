// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Journal/AtlantisJournalSubsystem.h"

#include "AtlantisAudioFactBridgeComponent.generated.h"

class AController;
class APawn;
class UAtlantisHidingComponent;
class UAtlantisOxygenComponent;

/**
 * Forwards existing gameplay events to the audio ecosystem as facts.
 * Only translates events into facts. How a fact sounds is decided by the music axis mapping.
 */
UCLASS(ClassGroup = (Atlantis), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisAudioFactBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Binds to the sources once the owner is locally controlled. Safe to call more than once. */
	void TryBindSources();
	void UnbindSources();

	/** Sends each fact's current value once, so the music is right from the start. */
	void SendInitialFacts();

	void SendOxygenLevel();
	void SendInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status);

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

	UFUNCTION()
	void HandleInvestigationUpdated(EJournalInvestigationStatus OldStatus, EJournalInvestigationStatus Status, const FString& Title);
	
	UFUNCTION()
	void HandleInvestigationsCleared();

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisOxygenComponent> OxygenComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisHidingComponent> HidingComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisJournalSubsystem> JournalSubsystem;

	bool bSourcesBound = false;

	/** Investigations already sent to the music. The journal has emptied itself by the time it says it cleared. */
	TSet<FString> SentInvestigationTitles;

};
