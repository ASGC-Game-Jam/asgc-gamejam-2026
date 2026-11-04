// Copyright (c) 2026 ASGC

#include "Audio/AtlantisAudioFactBridgeComponent.h"

#include "Audio/AtlantisAudioDataBPFL.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Core/AtlantisHidingComponent.h"
#include "Core/AtlantisOxygenComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "StructUtils/InstancedStruct.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioFactBridge, Log, All);



void UAtlantisAudioFactBridgeComponent::BeginPlay()
{
    Super::BeginPlay();

    APawn* Pawn = Cast<APawn>(GetOwner());
    if (!Pawn)
    {
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("Fact bridge on [%s] must be on a pawn. No gameplay facts will reach the music."), *GetNameSafe(GetOwner()));
		return;
	}

	// The pawn can begin play before it's possessed, so try again when its controller changes.
	Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::HandleControllerChanged);
	TryBindSources();
}

void UAtlantisAudioFactBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged);
    }

    GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    UnbindSources();

    Super::EndPlay(EndPlayReason);

}


void UAtlantisAudioFactBridgeComponent::TryBindSources()
{
    if (bSourcesBound)
    {
        return;
    }

    APawn* Pawn = Cast<APawn>(GetOwner());
    if(!Pawn || !Pawn->IsLocallyControlled())
    {
        return;
    }

    AAtlantisPlayerState* PlayerState = Pawn->GetPlayerState<AAtlantisPlayerState>();
    if (!PlayerState)
    {
        GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TryBindSources);
        return;
    }

    bSourcesBound = true;

    OxygenComponent = PlayerState->GetOxygenComponent();
    if (OxygenComponent)
    {
        OxygenComponent->OnCurrentOxygenChanged.AddDynamic(this, &ThisClass::HandleCurrentOxygenChanged);
        OxygenComponent->OnCriticalOxygenChanged.AddDynamic(this, &ThisClass::HandleCriticalOxygenChanged);
        OxygenComponent->OnAllocatedOxygenChanged.AddDynamic(this, &ThisClass::HandleAllocatedOxygenChanged);
		OxygenComponent->OnOxygenCapacityChanged.AddDynamic(this, &ThisClass::HandleOxygenCapacityChanged);
    }
    else
    {
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No oxygen component on [%s]. Oxygen facts won't reach the music."), *GetNameSafe(PlayerState));
	}

    HidingComponent = Pawn->FindComponentByClass<UAtlantisHidingComponent>();
    if (HidingComponent)
    {
        HidingComponent->OnHidingStateChanged.AddDynamic(this, &ThisClass::HandleHidingStateChanged);
    }
    else
	{
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No hiding component on [%s]. Hiding facts won't reach the music."), *GetNameSafe(Pawn));
	}

    const UGameInstance* GameInstance = GetWorld()->GetGameInstance();
    JournalSubsystem = GameInstance ? GameInstance->GetSubsystem<UAtlantisJournalSubsystem>() : nullptr;
    if (JournalSubsystem)
    {
        JournalSubsystem->OnInvestigationUpdated.AddDynamic(this, &ThisClass::HandleInvestigationUpdated);
    }
	else
	{
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No journal subsystem found. Investigation facts won't reach the music."));
	}

	SendInitialFacts();
}

void UAtlantisAudioFactBridgeComponent::UnbindSources()
{
    if (OxygenComponent)
    {
        OxygenComponent->OnCurrentOxygenChanged.RemoveDynamic(this, &ThisClass::HandleCurrentOxygenChanged);
        OxygenComponent->OnCriticalOxygenChanged.RemoveDynamic(this, &ThisClass::HandleCriticalOxygenChanged);
        OxygenComponent->OnAllocatedOxygenChanged.RemoveDynamic(this, &ThisClass::HandleAllocatedOxygenChanged);
		OxygenComponent->OnOxygenCapacityChanged.RemoveDynamic(this, &ThisClass::HandleOxygenCapacityChanged);
        OxygenComponent = nullptr;
    }

    if (HidingComponent)
    {
        HidingComponent->OnHidingStateChanged.RemoveDynamic(this, &ThisClass::HandleHidingStateChanged);
        HidingComponent = nullptr;
    }

    if (JournalSubsystem)
    {
        JournalSubsystem->OnInvestigationUpdated.RemoveDynamic(this, &ThisClass::HandleInvestigationUpdated);
        JournalSubsystem = nullptr;
    }

    bSourcesBound = false;

}


void UAtlantisAudioFactBridgeComponent::SendInitialFacts()
{
    if (OxygenComponent)
    {
        SendOxygenLevel();
        HandleCriticalOxygenChanged(OxygenComponent->IsCriticalOxygen());
    }

    if (HidingComponent)
    {
        HandleHidingStateChanged(HidingComponent->IsHiding());
    }

    if (JournalSubsystem)
    {
        for (const FString& Title : JournalSubsystem->GetInvestigations())
        {
            SendInvestigationStatus(Title, JournalSubsystem->GetInvestigationStatus(Title));
        }
    }
}

void UAtlantisAudioFactBridgeComponent::SendOxygenLevel()
{
    if (!OxygenComponent)
    {
        return;
    }

    const float Capacity = OxygenComponent->GetOxygenCapacity();

    FAtlantisAudioFloatPayload Payload;
    Payload.Value = Capacity > 0.f ? FMath::Clamp(OxygenComponent->GetAvailableOxygen() / Capacity, 0.f, 1.f) : 0.f;

    UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Oxygen_Level, FInstancedStruct::Make(Payload));

}

void UAtlantisAudioFactBridgeComponent::SendInvestigationStatus(const FString& Title, EJournalInvestigationStatus Status)
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
    }

    UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Investigation_Status, FInstancedStruct::Make(Payload));
}

void UAtlantisAudioFactBridgeComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
    TryBindSources();
}

void UAtlantisAudioFactBridgeComponent::HandleCurrentOxygenChanged(float OldCurrentOxygen, float NewCurrentOxygen)
{
    SendOxygenLevel();
}

void UAtlantisAudioFactBridgeComponent::HandleCriticalOxygenChanged(bool bCriticalOxygen)
{
    FAtlantisAudioBoolPayload Payload;
    Payload.bValue = bCriticalOxygen;

    UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Oxygen_Critical, FInstancedStruct::Make(Payload));
}

void UAtlantisAudioFactBridgeComponent::HandleAllocatedOxygenChanged(float OldAllocatedOxygen, float NewAllocatedOxygen)
{
    SendOxygenLevel();
}

void UAtlantisAudioFactBridgeComponent::HandleOxygenCapacityChanged(float OldOxygenCapacity, float NewOxygenCapacity)
{
    SendOxygenLevel();
}

void UAtlantisAudioFactBridgeComponent::HandleHidingStateChanged(bool bHiding)
{
    FAtlantisAudioBoolPayload Payload;
    Payload.bValue = bHiding;

    UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Stealth_Hiding, FInstancedStruct::Make(Payload));
}

void UAtlantisAudioFactBridgeComponent::HandleInvestigationUpdated(EJournalInvestigationStatus OldStatus, EJournalInvestigationStatus Status, const FString& Title)
{
    SendInvestigationStatus(Title,Status);
}
