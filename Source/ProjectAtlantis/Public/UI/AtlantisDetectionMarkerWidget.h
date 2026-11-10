// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisDetectionMarkerWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * Above-head detection marker: a fill bar for how close the enemy is to spotting the player, plus a '?' / '!' icon.
 * Works with no Blueprint graph: name a Progress Bar "FillBar" and a Text Block "StatusText" in the Widget Blueprint
 * and this class drives them. Override OnDetectionUpdated in the Widget Blueprint for extra effects.
 */
UCLASS(Abstract)
class PROJECTATLANTIS_API UAtlantisDetectionMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Pushes new detection values to the marker. Called by UAtlantisDetectionMarkerComponent. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Detection")
	void SetDetection(float InMeter, EMonsterStatus InStatus);

	/** Detection meter, 0-1. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	float GetMeter() const { return Meter; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	EMonsterStatus GetStatus() const { return Status; }

	/** How full the bar is drawn: the meter, or full while in Combat. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	float GetDisplayFill() const;

	/** Bar colour for the current status. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	FLinearColor GetStatusColor() const;

protected:
	virtual void NativeConstruct() override;

	/** Called after every update (and once on construct). Use for extra effects; the bar and icon are already handled. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Atlantis|AI|Detection")
	void OnDetectionUpdated(float InMeter, EMonsterStatus InStatus);

	/** Optional: fill bar. Fill colour follows the status colours below. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|AI|Detection", meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> FillBar;

	/** Optional: shows '?' while suspicious (Alerted / Searching) and '!' in Combat. */
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|AI|Detection", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Bar colour while Idle (meter filling but the enemy hasn't reacted yet). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection")
	FLinearColor IdleColor = FLinearColor(1.f, 1.f, 1.f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection")
	FLinearColor AlertedColor = FLinearColor(1.f, 0.85f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection")
	FLinearColor SearchingColor = FLinearColor(1.f, 0.5f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection")
	FLinearColor CombatColor = FLinearColor(1.f, 0.1f, 0.05f, 1.f);

private:
	void Refresh();

	float Meter = 0.f;
	EMonsterStatus Status = EMonsterStatus::Idle;
};
