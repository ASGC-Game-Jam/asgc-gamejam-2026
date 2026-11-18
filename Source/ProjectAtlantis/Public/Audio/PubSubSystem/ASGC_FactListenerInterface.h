// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ASGC_FactListenerInterface.generated.h"

/** Implement this to receive gameplay facts from the data router. */
UINTERFACE(MinimalAPI, Blueprintable)
class UASGC_FactListenerInterface : public UInterface
{
    GENERATED_BODY()
};

class PROJECTATLANTIS_API IASGC_FactListenerInterface
{
    GENERATED_BODY()

public:
    /** Called when a fact this object subscribed to is broadcast. Override it in C++, or as Event OnGameplayFactReceived in Blueprint. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ASGC|PubSub")
    void OnGameplayFactReceived(FGameplayTag Tag, const FInstancedStruct& Payload);
};