// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisSightConeComponent.generated.h"

class UAIPerceptionComponent;

/**
 * Debug visualizer for an AI's sight sense. Has no logic of its own: the owner calls DrawSightCone()
 * with its perception component, the pawn it sees through, and its current status.
 * Draws nothing in Shipping builds.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisSightConeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisSightConeComponent();

	/** Draws the sight cone for this frame from the pawn's eyes, using the perception component's sight config. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Debug")
	void DrawSightCone(const UAIPerceptionComponent* PerceptionComponent, const APawn* ViewPawn, EMonsterStatus Status) const;

	/** Master toggle. When off, DrawSightCone does nothing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug")
	bool bDrawSightCone = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug")
	FColor IdleColor = FColor::Green;

	/** Used for Alerted and Searching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug")
	FColor SuspiciousColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug")
	FColor DetectedColor = FColor::Red;

	/**
	 * Also draw a fainter outer cone at LoseSightRadius. Once a target has been seen,
	 * the AI keeps seeing it out to this range, not just to SightRadius.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug")
	bool bDrawLoseSightCone = true;

	/** How bright the outer lose-sight cone is compared to the main cone (0 = black, 1 = same). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug", meta = (ClampMin = 0, ClampMax = 1, UIMin = 0, UIMax = 1, EditCondition = "bDrawLoseSightCone"))
	float LoseSightConeBrightness = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug", meta = (ClampMin = 4, UIMin = 4))
	int32 NumSides = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|AI|Debug", meta = (ClampMin = 0, UIMin = 0))
	float LineThickness = 1.5f;

private:
	FColor GetColorForStatus(EMonsterStatus Status) const;
};
