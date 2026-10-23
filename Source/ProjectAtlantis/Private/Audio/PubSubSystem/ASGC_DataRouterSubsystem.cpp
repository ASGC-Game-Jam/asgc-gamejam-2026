// Copyright (c) 2026 ASGC

#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"

DEFINE_LOG_CATEGORY(AtlantisPubSubLogCategory);

void UASGC_DataRouterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    UE_LOG(AtlantisPubSubLogCategory, Log, TEXT("ASGC Audio Data Router Subsystem Initialized for World: %s"), *GetWorld()->GetName());
}

void UASGC_DataRouterSubsystem::Deinitialize()
{
    UE_LOG(AtlantisPubSubLogCategory, Log, TEXT("ASGC Audio Data Router Subsystem Deinitializing. Clearing %d subscription lists."), Subscriptions.Num());
    Subscriptions.Empty();
    Super::Deinitialize();
}

int32 UASGC_DataRouterSubsystem::Subscribe(FGameplayTag Tag, UObject* Subscriber)
{
    if (!Tag.IsValid())
    {
        ensureMsgf(false, TEXT("UASGC_DataRouterSubsystem::Subscribe: Attempted to subscribe with an invalid GameplayTag."));
        UE_LOG(AtlantisPubSubLogCategory, Error, TEXT("Subscribe failed: Invalid GameplayTag provided."));
        return INDEX_NONE;
    }

    if (!Subscriber)
    {
        ensureMsgf(false, TEXT("UASGC_DataRouterSubsystem::Subscribe: Attempted to subscribe with a null Subscriber UObject."));
        UE_LOG(AtlantisPubSubLogCategory, Error, TEXT("Subscribe failed for tag [%s]: Subscriber UObject is null."), *Tag.ToString());
        return INDEX_NONE;
    }

    // Verify the subscriber implements our listener interface (supports both C++ and Blueprints)
    if (!Subscriber->GetClass()->ImplementsInterface(UASGC_FactListenerInterface::StaticClass()))
    {
        ensureMsgf(false, TEXT("UASGC_DataRouterSubsystem::Subscribe: Subscriber [%s] does not implement IASGC_FactListenerInterface!"), *Subscriber->GetName());
        UE_LOG(AtlantisPubSubLogCategory, Error, TEXT("Subscribe failed for tag [%s]: Object [%s] does not implement IASGC_FactListenerInterface."), *Tag.ToString(), *Subscriber->GetName());
        return INDEX_NONE;
    }

    const int32 AssignedHandle = ++NextHandleID;

    FGameplayFactSubscriberRecord Record;
    Record.SubscriberObject = Subscriber;
    Record.HandleID = AssignedHandle;

    Subscriptions.FindOrAdd(Tag).Add(Record);

    UE_LOG(AtlantisPubSubLogCategory, Verbose, TEXT("Subscriber [%s] successfully subscribed to Tag [%s] with Handle ID: %d"), 
        *Subscriber->GetName(), *Tag.ToString(), AssignedHandle);

    return AssignedHandle;
}

void UASGC_DataRouterSubsystem::Unsubscribe(FGameplayTag Tag, int32 Handle)
{
    if (!Tag.IsValid() || Handle == INDEX_NONE)
    {
        return;
    }

    if (TArray<FGameplayFactSubscriberRecord>* List = Subscriptions.Find(Tag))
    {
        int32 NumRemoved = List->RemoveAll([Handle](const FGameplayFactSubscriberRecord& Record) {
            return Record.HandleID == Handle;
        });

        if (NumRemoved == 0)
        {
            ensureMsgf(false, TEXT("UASGC_DataRouterSubsystem::Unsubscribe: Failed to find subscriber record with Handle %d for Tag %s."), Handle, *Tag.ToString());
            UE_LOG(AtlantisPubSubLogCategory, Warning, TEXT("Unsubscribe warning: Handle %d not found under Tag [%s]."), Handle, *Tag.ToString());
        }
        else
        {
            UE_LOG(AtlantisPubSubLogCategory, Verbose, TEXT("Successfully unsubscribed handle %d from Tag [%s]"), Handle, *Tag.ToString());
        }

        if (List->Num() == 0)
        {
            Subscriptions.Remove(Tag);
        }
    }
    else
    {
        ensureMsgf(false, TEXT("UASGC_DataRouterSubsystem::Unsubscribe: Attempted to unsubscribe from Tag %s which has no active subscriptions."), *Tag.ToString());
    }
}

void UASGC_DataRouterSubsystem::BroadcastFact(FGameplayTag Tag, const FInstancedStruct& Payload)
{
    if (!Tag.IsValid())
    {
        return;
    }

    TArray<FGameplayFactSubscriberRecord>* List = Subscriptions.Find(Tag);
    if (!List || List->Num() == 0)
    {
        // A tag with no subscribers does nothing and raises no error.
        return;
    }

    // Take a snapshot copy to safely handle potential subscription modifications during callbacks
    TArray<FGameplayFactSubscriberRecord> SubscribersSnapshot = *List;

    for (const FGameplayFactSubscriberRecord& Record : SubscribersSnapshot)
    {
        bool bIsSubscriberValid = Record.SubscriberObject != nullptr && IsValid(Record.SubscriberObject);

        if (!bIsSubscriberValid)
        {
            // Fail loudly per requirements: leaked dead object that didn't unsubscribe
            ensureMsgf(false, 
                TEXT("UASGC_DataRouterSubsystem::BroadcastFact: Subscriber for Tag [%s] (Handle: %d) is invalid or garbage collected without explicit unsubscription! Lifecycle bug detected."), 
                *Tag.ToString(), Record.HandleID);
            
            UE_LOG(AtlantisPubSubLogCategory, Error, TEXT("Lifecycle Error: Dead subscriber detected on Tag [%s] with Handle %d."), *Tag.ToString(), Record.HandleID);
            continue;
        }

        // Execute the interface function (works seamlessly for both C++ classes and Blueprints)
        IASGC_FactListenerInterface::Execute_OnGameplayFactReceived(Record.SubscriberObject, Tag, Payload);
    }
}