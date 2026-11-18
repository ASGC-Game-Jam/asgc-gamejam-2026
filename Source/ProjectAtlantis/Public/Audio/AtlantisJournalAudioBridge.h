// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Journal/AtlantisJournalSubsystem.h"

#include "AtlantisJournalAudioBridge.generated.h"

/**
 * Sends journal investigations to the audio ecosystem as Audio.Fact.Investigation.Status facts.
 * Lives on the world rather than a pawn, so it never misses a change between possessions.
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
	/** Only game and PIE worlds, because editor viewport worlds have no journal. */
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	/** Sends one investigation's state, and remembers its title for when the journal clears. */
	void SendInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status);

	UFUNCTION()
	void HandleInvestigationUpdated(EJournalInvestigationStatus OldStatus, EJournalInvestigationStatus Status, const FString& Title);

	/** Sends every remembered investigation as Undiscovered, then forgets them. */
	UFUNCTION()
	void HandleInvestigationsCleared();

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisJournalSubsystem> Journal;

	/** Titles already sent. Kept because the journal is already empty by the time it reports a clear. */
	TSet<FString> SentInvestigationTitles;
};
