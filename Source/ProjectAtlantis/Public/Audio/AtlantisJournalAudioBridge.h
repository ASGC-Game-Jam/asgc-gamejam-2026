// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Journal/AtlantisJournalSubsystem.h"

#include "AtlantisJournalAudioBridge.generated.h"

/**
 * Reports journal investigations to the music as Audio.Fact.Investigation.Status facts, for the whole level.
 * Lives on the world rather than a pawn, because the journal belongs to the game instance.
 * This way it keeps listening while no pawn is possessed, so a journal change between possessions is never missed.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalAudioBridge : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Binds to the journal and sends every investigation's current state, so the music is right from the start. */
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	/** Unbinds from the journal, which outlives every level. */
	virtual void Deinitialize() override;

protected:
	/** Only game and PIE worlds. Editor viewport worlds have no game instance or music. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Sends one investigation's state to the music, and remembers its title for when the journal clears. */
	void SendInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status);

	UFUNCTION()
	void HandleInvestigationUpdated(EJournalInvestigationStatus OldStatus, EJournalInvestigationStatus Status, const FString& Title);

	/** Sends every remembered investigation as Undiscovered, then forgets them. */
	UFUNCTION()
	void HandleInvestigationsCleared();

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisJournalSubsystem> Journal;

	/** Investigations already sent to the music. The journal has emptied itself by the time it says it cleared. */
	TSet<FString> SentInvestigationTitles;
};
