// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "EngineSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UsefulHelpers.generated.h"

/**
 * Expose useful helper functions to Blueprint that are not currently available by default
 */
UCLASS()
class PROJECTATLANTIS_API UUsefulHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	UFUNCTION(BlueprintPure)
		static void GetProjectVersion(FString& version);
};
