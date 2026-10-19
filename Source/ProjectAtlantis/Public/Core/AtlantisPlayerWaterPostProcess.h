// Copyright (c) 2026 ASGC
#pragma once
#include "CoreMinimal.h"
#include "Engine/BlendableInterface.h"
#include "GameFramework/Actor.h"
#include "AtlantisPlayerWaterPostProcess.generated.h"

class APostProcessVolume;
class UMaterialInterface;
class UPostProcessComponent;
class APawn;

/** Material-only, per-view water effect. Authored volume bounds and player water state determine inclusion. */
UCLASS()
class PROJECTATLANTIS_API AAtlantisPlayerWaterPostProcess : public AActor, public IBlendableInterface
{
	GENERATED_BODY()
public:
	AAtlantisPlayerWaterPostProcess();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Atlantis|Water")
	TObjectPtr<UPostProcessComponent> PostProcess;
	/** Retains the original authored region; its other settings remain on the original volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Atlantis|Water")
	TObjectPtr<APostProcessVolume> SourceVolume;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Atlantis|Water")
	TObjectPtr<UMaterialInterface> UnderwaterMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Atlantis|Water", meta=(ClampMin="0", ClampMax="1"))
	float MaterialWeight = 1.f;
	virtual void OverrideBlendableSettings(FSceneView& View, float Weight) const override;
private:
	friend class FAtlantisPlayerWaterPostProcessSpec;
	APawn* ResolveViewPawn(const FSceneView& View) const;
	float GetPlayerBlendWeight(const APawn* Pawn) const;
};
