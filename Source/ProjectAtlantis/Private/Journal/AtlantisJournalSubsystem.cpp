// Copyright (c) 2026 ASGC


#include "Journal/AtlantisJournalSubsystem.h"

void UAtlantisJournalSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Log, TEXT("UAtlantisJournalSubsystem has initialized"));
}

void UAtlantisJournalSubsystem::Deinitialize()
{
    Super::Deinitialize();

    UE_LOG(LogTemp, Log, TEXT("UAtlantisJournalSubsystem has deinitalized!"));
}

void UAtlantisJournalSubsystem::OpenJournal()
{
    OnJournalOpen.Broadcast();
}

void UAtlantisJournalSubsystem::CloseJournal()
{
    OnJournalClose.Broadcast();
}

void UAtlantisJournalSubsystem::SignalJournal()
{
    OnJournalSignaled.Broadcast();
}

void UAtlantisJournalSubsystem::UpdateInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status)
{
    FJournalInvestigationEntry* Entry = Investigations.Find(Title);
    EJournalInvestigationStatus OldStatus = EJournalInvestigationStatus::Undiscovered;
    if (!Entry)
    {
        // entry not found, add a new one
        FJournalInvestigationEntry NewEntry = { .Title = Title, .Status = Status };
        Investigations.Add(Title, NewEntry);
    }
    else
    {
        // update description and status
        OldStatus = Entry->Status;
        Entry->Status = Status;
    }

    UE_LOG(LogTemp, Log, TEXT("UAtlantisJournalSubsystem signaling OnInvestigationUpdated from UpdateInvestigation"));
    OnInvestigationUpdated.Broadcast(OldStatus, Status, Title);
}


void UAtlantisJournalSubsystem::UpdateInvestigationDescription(const FString& Title, const FString& Description)
{
    FJournalInvestigationEntry* Entry = Investigations.Find(Title);
    EJournalInvestigationStatus OldStatus = EJournalInvestigationStatus::Undiscovered;
    if (!Entry)
    {
        // entry not found, add a new one
        FJournalInvestigationEntry NewEntry = { .Title = Title };
        NewEntry.Descriptions.Add(Description);
        Investigations.Add(Title, NewEntry);
    }
    else
    {
        // update description and status
        Entry->Descriptions.Add(Description);
        OldStatus = Entry->Status;
    }

    UE_LOG(LogTemp, Log, TEXT("UAtlantisJournalSubsystem signaling OnInvestigationUpdated from UpdateInvestigation"));
    OnInvestigationUpdated.Broadcast(OldStatus, OldStatus, Title);
}

void UAtlantisJournalSubsystem::UpdateInvestigationDescriptions(const FString& Title, TArray<FString> Descriptions)
{
    FJournalInvestigationEntry* Entry = Investigations.Find(Title);
    EJournalInvestigationStatus OldStatus = EJournalInvestigationStatus::Undiscovered;
    if (!Entry)
    {
        // entry not found, add a new one
        FJournalInvestigationEntry NewEntry = { .Title = Title };
        for (int32 Index = 0; Index < Descriptions.Num(); ++Index)
        {
            NewEntry.Descriptions.Add(Descriptions[Index]);
        }
        Investigations.Add(Title, NewEntry);
    }
    else
    {
        // update description and status
        for (int32 Index = 0; Index < Descriptions.Num(); ++Index)
        {
            Entry->Descriptions.Add(Descriptions[Index]);
        }
        OldStatus = Entry->Status;
    }

    UE_LOG(LogTemp, Log, TEXT("UAtlantisJournalSubsystem signaling OnInvestigationUpdated from UpdateInvestigationDescriptions"));
    OnInvestigationUpdated.Broadcast(OldStatus, OldStatus, Title);
}

TArray<FString> UAtlantisJournalSubsystem::GetInvestigations()
{
    // TODO this should return a list of localized titles strings
    TArray<FString> Titles;
    Investigations.GetKeys(Titles);
    return Titles;
}

EJournalInvestigationStatus UAtlantisJournalSubsystem::GetInvestigationStatus(const FString& Title)
{
    FJournalInvestigationEntry* Entry = Investigations.Find(Title);
    if (Entry)
    {
        return Entry->Status;
    }

    return EJournalInvestigationStatus::Undiscovered;
}

TArray<FString> UAtlantisJournalSubsystem::GetInvestigationDescriptions(const FString& Title)
{
    // TODO this should return a list of localized description strings
    FJournalInvestigationEntry* Entry = Investigations.Find(Title);
    if (Entry)
    {
        return Entry->Descriptions;
    }
    return { };
}

void UAtlantisJournalSubsystem::ClearInvestigations()
{
    Investigations.Reset();
    OnJournalInvestigationsCleared.Broadcast();
}

void UAtlantisJournalSubsystem::ClearAll()
{
    ClearInvestigations();
    OnJournalCleared.Broadcast();
}