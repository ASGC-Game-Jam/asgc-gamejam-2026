// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AtlantisJournalSubsystem.generated.h"

UENUM(BlueprintType)
enum class EJournalInvestigationStatus : uint8
{
	Undiscovered,
	Inprogress,
	Complete
};

UENUM(BlueprintType)
enum class EJournalEntryKind : uint8
{
	Clue,
	PuzzleObservation
};

/** Owner-supplied journal content, stored once in the existing journal. */
USTRUCT(BlueprintType)
struct FJournalInvestigationEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	TArray<FText> Descriptions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	EJournalInvestigationStatus Status = EJournalInvestigationStatus::Undiscovered;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FName EntryId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	EJournalEntryKind Kind = EJournalEntryKind::Clue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FName DistrictId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FText DistrictTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FName MysteryId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal")
	FText MysteryTitle;
};

// @note(Tan): params here are subject to change depending on how investigation data looks like
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnJournalInvestigationUpdateEvent, EJournalInvestigationStatus, OldStatus, EJournalInvestigationStatus, Status, const FString&, Title);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJournalOpenCloseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJournalSignaledEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJournalClearedEvent);

/**
 * Journal Subsystem
 * Manages events to be handled from other systems for UI journal to be updated
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	/** Add or replace owner-supplied content by stable identity. Does not decide discovery. */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	bool SetInvestigationEntry(const FJournalInvestigationEntry& Entry);

	/** Read all entries; the view decides which owner-supplied statuses to display. */
	UFUNCTION(BlueprintPure, Category = "Journal")
	TArray<FJournalInvestigationEntry> GetInvestigationEntries() const;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Signals journal to close UI
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void OpenJournal();

	// Signals journal to close UI
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void CloseJournal();

	// Event signaled whenever journal has entered open state
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalOpenCloseEvent OnJournalOpen;

	// Event signaled whenever journal has entered closed state
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalOpenCloseEvent OnJournalClose;

	// Signals journal to light up UI should be opened by player
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void SignalJournal();

	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalSignaledEvent OnJournalSignaled;

	// TODO Figure out how the data is formatted from investigation
	//		Stub function for investigation to update status of a specific of a journal entry with a single description
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void UpdateInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status);

	// TODO Figure out how the data is formatted from investigation
	//		Stub function for investigation to update status of a specific of a journal entry with a single description
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void UpdateInvestigationDescription(const FString& Title, const FString& Description);

	/** Replace the entry's description snapshot, preserving its identity and status. */
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void UpdateInvestigationDescriptions(const FString& Title, TArray<FString> Descriptions);

	// Event signaled whenever a journal entry has been updated
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalInvestigationUpdateEvent OnInvestigationUpdated;

	// Get list of investigations
	UFUNCTION(BlueprintCallable, Category = "Journal")
	TArray<FString> GetInvestigations();

	// Gets current status of investigation
	UFUNCTION(BlueprintCallable, Category = "Journal")
	EJournalInvestigationStatus GetInvestigationStatus(const FString& Title);

	// Gets a list of descriptions for a given Title
	UFUNCTION(BlueprintCallable, Category = "Journal")
	TArray<FString> GetInvestigationDescriptions(const FString& Title);

	// Clears out all investigations, should be used whenever returning to main menu or starting a new run
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void ClearInvestigations();

	// Event signaled whenever ClearInvestigations() is called
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalClearedEvent OnJournalInvestigationsCleared;

	// Catchall for clearing all types of journal entry types
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void ClearAll();

	// Event signaled whenever ClearAll() is called
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalClearedEvent OnJournalCleared;

private:
	
	UPROPERTY()
	TMap<FString, FJournalInvestigationEntry> Investigations;
};
