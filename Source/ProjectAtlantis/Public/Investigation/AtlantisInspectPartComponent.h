// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "AtlantisInspectPartComponent.generated.h"

UCLASS(ClassGroup=(Atlantis), meta=(BlueprintSpawnableComponent, DisplayName="Atlantis Inspect Part"))
class PROJECTATLANTIS_API UAtlantisInspectPartComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	bool IsPartEnabled() const { return bPartEnabled; }

	/** Disabled parts can't be focused, e.g. one that should only count after something else is found */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Inspection")
	void SetPartEnabled(bool bEnabled) { bPartEnabled = bEnabled; }

protected:
	/** Whether the part starts focusable. Can be changed at runtime with SetPartEnabled */
	UPROPERTY(EditAnywhere, Category = "Atlantis|Inspection", meta = (DisplayName = "Enabled"))
	bool bPartEnabled = true;
};
