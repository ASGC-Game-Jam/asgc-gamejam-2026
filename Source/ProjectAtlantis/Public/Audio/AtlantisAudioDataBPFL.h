// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "AtlantisAudioDataBPFL.generated.h"

/**
 * Front door for gameplay facts into the Audio Ecosystem (TDD §9).
 * Validates the tag against the payload registry, then hands the payload to the router.
 * Never references a consumer.
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisAudioDataBPFL : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
	/** Sends a gameplay fact to whoever is listening for Tag. Returns false (and logs an error) if the fact was rejected. */
    UFUNCTION(BlueprintCallable, Category = "Atlantis Audio Ecosystem|Ingestion", meta = (WorldContext = "WorldContextObject"))
    static bool SetAudioDataPayload(const UObject* WorldContextObject, FGameplayTag Tag, const FInstancedStruct& Payload);
};