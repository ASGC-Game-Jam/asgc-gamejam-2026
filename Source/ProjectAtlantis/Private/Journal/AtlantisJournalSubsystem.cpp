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
        FJournalInvestigationEntry NewEntry = { .Title = FText::FromString(Title), .Status = Status };
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
        FJournalInvestigationEntry NewEntry = { .Title = FText::FromString(Title) };
        NewEntry.Descriptions.Add(FText::FromString(Description));
        Investigations.Add(Title, NewEntry);
    }
    else
    {
        // update description and status
        Entry->Descriptions.Add(FText::FromString(Description));
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
        FJournalInvestigationEntry NewEntry = { .Title = FText::FromString(Title) };
        for (int32 Index = 0; Index < Descriptions.Num(); ++Index)
        {
            NewEntry.Descriptions.Add(FText::FromString(Descriptions[Index]));
        }
        Investigations.Add(Title, NewEntry);
    }
    else
    {
        // Replace the owner's current snapshot instead of retaining outdated descriptions.
        Entry->Descriptions.Reset();
        for (int32 Index = 0; Index < Descriptions.Num(); ++Index)
        {
            Entry->Descriptions.Add(FText::FromString(Descriptions[Index]));
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
        TArray<FString> Descriptions;
        for (const FText& Description : Entry->Descriptions)
        {
            Descriptions.Add(Description.ToString());
        }
        return Descriptions;
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

bool UAtlantisJournalSubsystem::SetInvestigationEntry(const FJournalInvestigationEntry& Entry)
{
	if (Entry.EntryId.IsNone() || Entry.Title.IsEmptyOrWhitespace()
		|| Entry.Status > EJournalInvestigationStatus::Complete || Entry.Kind > EJournalEntryKind::PuzzleObservation)
	{
		return false;
	}

	FString Key = Entry.EntryId.ToString();
	// FName identity is case-insensitive; keep the original key for legacy string callers.
	for (const auto& Existing : Investigations)
	{
		if (!Existing.Value.EntryId.IsNone() && Existing.Value.EntryId == Entry.EntryId)
		{
			Key = Existing.Key;
			break;
		}
	}
	const EJournalInvestigationStatus OldStatus = GetInvestigationStatus(Key);
	Investigations.Add(Key, Entry);
	OnInvestigationUpdated.Broadcast(OldStatus, Entry.Status, Key);
	return true;
}

TArray<FJournalInvestigationEntry> UAtlantisJournalSubsystem::GetInvestigationEntries() const
{
	TArray<FString> Keys;
	Investigations.GetKeys(Keys);
	Keys.Sort();

	TArray<FJournalInvestigationEntry> Entries;
	Entries.Reserve(Keys.Num());
	for (const FString& Key : Keys)
	{
		FJournalInvestigationEntry Entry = Investigations[Key];
		if (Entry.EntryId.IsNone())
		{
			Entry.EntryId = FName(*Key);
		}
		Entries.Add(MoveTemp(Entry));
	}
	return Entries;
}
