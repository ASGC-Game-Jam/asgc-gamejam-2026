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

USTRUCT()
struct FJournalInvestigationEntry
{
	GENERATED_BODY()

	FString Title;
	TArray<FString> Descriptions;
	EJournalInvestigationStatus Status;
};

// @note(Tan): params here are subject to change depending on how investigation data looks like
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnJournalUpdateEvent, EJournalInvestigationStatus, Status, const FString&, Title);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJournalOpenCloseEvent);

/**
 * Journal Subsystem
 * Manages events to be handled from other systems for UI journal to be updated
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// Event signaled whenever journal has entered open state
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalOpenCloseEvent OnJournalOpen;

	// Event signaled whenever journal has entered closed state
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalOpenCloseEvent OnJournalClose;

	// TODO Figure out how the data is formatted from investigation
	//		Stub function for investigation to update status of a specific of a journal entry with a single description
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void UpdateInvestigation(EJournalInvestigationStatus Status, const FString& Title, const FString& Description);

	// TODO Figure out how the data is formatted from investigation
	//		Stub function for investigation to update status of a specific of a journal entry with a list of descriptions
	//      can be used for scenarios such as after game loads and investigation system wants to update entire state of a
	//      entry
	UFUNCTION(BlueprintCallable, Category = "Journal")
	void UpdateInvestigationDescriptions(EJournalInvestigationStatus Status, const FString& Title, TArray<FString> Descriptions);

	// Event signaled whenever a journal entry has been updated
	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalUpdateEvent OnInvestigationUpdated;

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

private:
	
	UPROPERTY()
	TMap<FString, FJournalInvestigationEntry> Investigations;
};
