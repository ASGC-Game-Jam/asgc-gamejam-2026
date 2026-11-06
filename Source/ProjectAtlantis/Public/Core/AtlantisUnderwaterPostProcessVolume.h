// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/PostProcessVolume.h"

#include "AtlantisUnderwaterPostProcessVolume.generated.h"

class AAtlantisPlayerState;
class APawn;
class APlayerState;

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
	void HandlePawnSet(APlayerState* Player, APawn* NewPawn, APawn* OldPawn);
	
	UFUNCTION()
	void HandleUnderwaterChanged(bool bUnderwater);
	
	void BindToPawn(APawn* Pawn);
	void UnbindFromPawn(APawn* Pawn);
	
	/** Non-owning; used to unbind on EndPlay if the PlayerState is still alive. */
	TWeakObjectPtr<AAtlantisPlayerState> BoundPlayerState;
};
