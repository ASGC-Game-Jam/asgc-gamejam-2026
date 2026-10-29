// Copyright (c) 2026 ASGC

#pragma once

#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"
#include "StructUtils/InstancedStruct.h"
#include "AtlantisAudioTestFactListener.generated.h"

/** Records the facts the router delivers during automation tests. */
UCLASS(Transient)
class UAtlantisAudioTestFactListener : public UObject, public IASGC_FactListenerInterface
{
	GENERATED_BODY()

public:
	int32 ReceivedCount = 0;
	FGameplayTag LastTag;
	FInstancedStruct LastPayload;

	virtual void OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload) override
	{
		++ReceivedCount;
		LastTag = Tag;
		LastPayload = Payload;
	}
};
