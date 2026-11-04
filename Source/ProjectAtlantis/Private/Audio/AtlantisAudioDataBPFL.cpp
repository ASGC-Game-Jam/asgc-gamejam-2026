// Copyright (c) 2026 ASGC

#include "Audio/AtlantisAudioDataBPFL.h"

#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Engine/Engine.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "GameFramework/Actor.h"
#include "StructUtils/InstancedStruct.h"


DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioIngestion, Log, All);


bool UAtlantisAudioDataBPFL::SetAudioDataPayload(const UObject* WorldContextObject, FGameplayTag Tag, const FInstancedStruct& Payload)
{
    if (!UAtlantisAudioDeveloperSettings::ValidatePayload(Tag, Payload))
    {
        return false;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
    if (!World)
    {
        UE_LOG(LogAtlantisAudioIngestion, Error, TEXT("SetAudioDataPayload for tag [%s] has no valid world. Call it from an object that lives in a level."), *Tag.ToString());
        return false;
    }

    UASGC_DataRouterSubsystem* DataRouter = World->GetSubsystem<UASGC_DataRouterSubsystem>();
    if (!DataRouter)
    {
        UE_LOG(LogAtlantisAudioIngestion, Error, TEXT("SetAudioDataPayload for tag [%s] couldn't find the Data Router subsystem in this world."), *Tag.ToString());
        return false;
    }

    // An empty Key means "this is about me", so fill it in from whoever called this node.
    const FAtlantisAudioKeyedStatePayload* KeyedData = Payload.GetPtr<FAtlantisAudioKeyedStatePayload>();
    if (KeyedData && KeyedData->Key.IsNone())
    {
        const AActor* OwningActor = Cast<AActor>(WorldContextObject);
        if (!OwningActor)
        {
            OwningActor = WorldContextObject->GetTypedOuter<AActor>();
        }

        FAtlantisAudioKeyedStatePayload FilledData = *KeyedData;
        FilledData.Key = OwningActor ? OwningActor->GetFName() : WorldContextObject->GetFName();

        DataRouter->BroadcastFact(Tag, FInstancedStruct::Make(FilledData));
        return true;
    }

    DataRouter->BroadcastFact(Tag, Payload);
    return true;
}