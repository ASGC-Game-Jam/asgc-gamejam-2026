// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AtlantisNavGraphLibrary.generated.h"

/** Blueprint/Python access to the swimming nav graph (world subsystems aren't reachable from Python directly). */
UCLASS()
class PROJECTATLANTIS_API UAtlantisNavGraphLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Rebuilds the nav graph of the given world and returns a summary (counts, isolated nodes, nodes inside geometry). */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Navigation", meta = (WorldContext = "WorldContextObject"))
	static FString BuildAndDescribeNavGraph(const UObject* WorldContextObject);
};
