// Copyright (c) 2026 ASGC

#include "Audio/AtlantisJournalAudioBridge.h"

#include "Audio/AtlantisAudioDataBPFL.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "StructUtils/InstancedStruct.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisJournalAudio, Log, All);

bool UAtlantisJournalAudioBridge::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAtlantisJournalAudioBridge::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    UGameInstance* GI = InWorld.GetGameInstance();

    Journal = GI ? GI->GetSubsystem<UAtlantisJournalSubsystem>() : nullptr;

    if (Journal)
    {
        Journal->OnInvestigationUpdated.AddDynamic(this, &ThisClass::HandleInvestigationUpdated);
        // Fires for both ClearInvestigations() and ClearAll().
        Journal->OnJournalInvestigationsCleared.AddDynamic(this, &ThisClass::HandleInvestigationsCleared);

        // Investigations can already exist when the level starts, such as after level travel.
        for (const FString& Title : Journal->GetInvestigations())
        {
            SendInvestigationStatus(Title, Journal->GetInvestigationStatus(Title));
        }
    }
	else
	{
        UE_LOG(LogAtlantisJournalAudio, Error, TEXT("No Journal Subsystem found. Investigation facts won't reach the music system."));
    }
}

void UAtlantisJournalAudioBridge::Deinitialize()
{
    // Nothing is withdrawn here: the music subsystem forgets every fact when the world is cleaned up.
    if (Journal)
    {
        Journal->OnInvestigationUpdated.RemoveDynamic(this, &ThisClass::HandleInvestigationUpdated);
        Journal->OnJournalInvestigationsCleared.RemoveDynamic(this, &ThisClass::HandleInvestigationsCleared);
        Journal = nullptr;
    }
    SentInvestigationTitles.Reset();

    Super::Deinitialize();
}


void UAtlantisJournalAudioBridge::SendInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status)
{
    FAtlantisAudioKeyedStatePayload Payload;
    Payload.Key = FName(*Title);

    switch (Status)
    {
        case EJournalInvestigationStatus::Undiscovered:
            Payload.State = TAG_AudioState_Investigation_Undiscovered;
            break;
        case EJournalInvestigationStatus::Inprogress:
            Payload.State = TAG_AudioState_Investigation_InProgress;
            break;
        case EJournalInvestigationStatus::Complete:
            Payload.State = TAG_AudioState_Investigation_Complete;
            break;
        default:
            // A new status was added to the journal without an Audio.State.Investigation tag to match.
            UE_LOG(LogAtlantisJournalAudio, Warning, TEXT("Investigation [%s] has a status with no audio state tag. Add a tag for it in AtlantisAudioFactTags."), *Title);
            return;
    }

    SentInvestigationTitles.Add(Title);

    UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Investigation_Status, FInstancedStruct::Make(Payload));
}

void UAtlantisJournalAudioBridge::HandleInvestigationUpdated(EJournalInvestigationStatus OldStatus, EJournalInvestigationStatus Status, const FString& Title)
{
    SendInvestigationStatus(Title, Status);
}

void UAtlantisJournalAudioBridge::HandleInvestigationsCleared()
{
    // The journal is already empty, so use the titles sent so far. Loop over a copy,
    // because SendInvestigationStatus adds each title back to the set.
    const TSet<FString> Titles = SentInvestigationTitles;

    for (const FString& Title : Titles)
    {
        SendInvestigationStatus(Title, EJournalInvestigationStatus::Undiscovered);
    }

    SentInvestigationTitles.Reset();
}