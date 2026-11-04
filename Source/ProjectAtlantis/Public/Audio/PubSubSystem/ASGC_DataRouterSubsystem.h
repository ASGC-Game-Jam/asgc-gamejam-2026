// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"


#include "ASGC_DataRouterSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(AtlantisPubSubLogCategory, Log, All);

// Internal record holding subscriber details for strict lifecycle tracking.
struct FGameplayFactSubscriberRecord
{
    TWeakObjectPtr<UObject> SubscriberObject = nullptr;
    int32 HandleID = INDEX_NONE;
};

/**
 * UASGC_DataRouterSubsystem
 * A world-scoped subsystem responsible for fanning out gameplay facts 
 * from the front door to active consumers by flat GameplayTag.
 */
UCLASS()
class PROJECTATLANTIS_API UASGC_DataRouterSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Subscribes a listener object (C++ or Blueprint implementing IASGC_FactListenerInterface) to a tag.
     */
    UFUNCTION(BlueprintCallable, Category = "PubSub Router")
    int32 Subscribe(FGameplayTag Tag, UObject* Subscriber);

    // Explicitly unsubscribes using the handle returned during subscription.
    UFUNCTION(BlueprintCallable, Category = "PubSub Router")
    void Unsubscribe(FGameplayTag Tag, int32 Handle);

    // Broadcasts a gameplay fact payload globally by tag to all current subscribers.
    UFUNCTION(BlueprintCallable, Category = "PubSub Router")
    void BroadcastFact(FGameplayTag Tag, const FInstancedStruct& Payload);

private:
    TMap<FGameplayTag, TArray<FGameplayFactSubscriberRecord>> Subscriptions;

    int32 NextHandleID = 0;
};