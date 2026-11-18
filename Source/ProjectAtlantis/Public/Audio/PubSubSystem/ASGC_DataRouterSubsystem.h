// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"


#include "ASGC_DataRouterSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(AtlantisPubSubLogCategory, Log, All);

// One subscription: who is listening, and the handle they unsubscribe with.
struct FGameplayFactSubscriberRecord
{
    TWeakObjectPtr<UObject> SubscriberObject = nullptr;
    int32 HandleID = INDEX_NONE;
};

/**
 * Delivers gameplay facts from the front door to every listener subscribed to their tag.
 * One per world, so subscriptions never outlive their level.
 */
UCLASS()
class PROJECTATLANTIS_API UASGC_DataRouterSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Subscribes a listener (C++ or Blueprint) to a tag. Returns the handle to unsubscribe with, or INDEX_NONE on failure. */
    UFUNCTION(BlueprintCallable, Category = "PubSub Router")
    int32 Subscribe(FGameplayTag Tag, UObject* Subscriber);

    /** Unsubscribes using the handle Subscribe returned. */
    UFUNCTION(BlueprintCallable, Category = "PubSub Router")
    void Unsubscribe(FGameplayTag Tag, int32 Handle);

    /** Delivers a fact to every listener subscribed to Tag. Facts with no listener are dropped. */
    UFUNCTION(BlueprintCallable, Category = "PubSub Router")
    void BroadcastFact(FGameplayTag Tag, const FInstancedStruct& Payload);

private:
    TMap<FGameplayTag, TArray<FGameplayFactSubscriberRecord>> Subscriptions;

    int32 NextHandleID = 0;
};