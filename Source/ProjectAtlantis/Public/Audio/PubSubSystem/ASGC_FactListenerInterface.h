// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "ASGC_FactListenerInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UASGC_FactListenerInterface : public UInterface
{
    GENERATED_BODY()
};

class PROJECTATLANTIS_API IASGC_FactListenerInterface
{
    GENERATED_BODY()

public:
    /**
     * Called when a subscribed gameplay fact is broadcasted.
     * Overridable in C++ and Blueprint (`Event OnGameplayFactReceived`).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ASGC|PubSub")
    void OnGameplayFactReceived(FGameplayTag Tag, const FInstancedStruct& Payload);
};