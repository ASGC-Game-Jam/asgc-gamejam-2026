// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisDetectionMarkerComponent.generated.h"

/**
 * Shows the enemy's detection meter above its head (screen space).
 * Add it to an enemy pawn and set Widget Class to a child of UAtlantisDetectionMarkerWidget (e.g. WBP_DetectionMarker).
 * AAtlantisEnemyController finds it on possess and pushes meter/status changes; nothing else to wire.
 */
UCLASS(ClassGroup = "Atlantis", meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisDetectionMarkerComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	UAtlantisDetectionMarkerComponent();

	/** Updates the marker. Hidden while Idle with an empty meter (when bHideWhenCalm). */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Detection")
	void SetDetection(float InMeter, EMonsterStatus InStatus);

	virtual void InitWidget() override;

protected:
	/** Hide the marker while the enemy is Idle and its meter is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection")
	bool bHideWhenCalm = true;

private:
	void PushToWidget();

	float Meter = 0.f;
	EMonsterStatus Status = EMonsterStatus::Idle;
	bool bHasLoggedWrongWidgetClass = false;
};
