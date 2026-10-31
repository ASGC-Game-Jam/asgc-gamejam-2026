// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"
#include "Core/EAtlantisTraversalMode.h"

#include "AtlantisUnderwaterPostProcessVolume.generated.h"

class AAtlantisPlayerState;

UCLASS()
class PROJECTATLANTIS_API AAtlantisUnderwaterPostProcessVolume : public APostProcessVolume
{
	GENERATED_BODY()
	
	
public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	/** Binds to the local player's traversal mode, retries next tick until the PlayerState exists. */
	void TryBindToPlayerState();
	
	UFUNCTION()
	void HandleTraversalModeChanged(EAtlantisTraversalMode OldTraversalMode, EAtlantisTraversalMode NewTraversalMode);
	
	/** Enables the volume only for water traversal modes */
	void ApplyTraversalMode(EAtlantisTraversalMode TraversalMode);
	
	/** Non-owning; used to unbind on EndPlay if the PlayerState is still alive. */
	TWeakObjectPtr<AAtlantisPlayerState> BoundPlayerState;
};
