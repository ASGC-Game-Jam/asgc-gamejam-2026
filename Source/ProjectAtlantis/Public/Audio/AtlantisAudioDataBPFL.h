// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "AtlantisAudioDataBPFL.generated.h"

/**
 * The front door for gameplay facts into the audio ecosystem (TDD §9).
 * Validates each fact against the payload registry, then hands it to the router.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisAudioDataBPFL : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
	/** Sends a gameplay fact to every listener subscribed to Tag. Returns false, and logs an error, if the fact is rejected. */
    UFUNCTION(BlueprintCallable, Category = "Atlantis Audio Ecosystem|Ingestion", meta = (WorldContext = "WorldContextObject"))
    static bool SetAudioDataPayload(const UObject* WorldContextObject, FGameplayTag Tag, const FInstancedStruct& Payload);
};