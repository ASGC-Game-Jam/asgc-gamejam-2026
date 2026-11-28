// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInspectPartFocusComponent.generated.h"

class UAtlantisInspectableComponent;
class UAtlantisInspectorComponent;
class UAtlantisInspectPartComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAtlantisPartFocusChanged,
	UAtlantisInspectPartComponent*, NewPart, UAtlantisInspectPartComponent*, OldPart);

UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Inspect Part Focus"))
class PROJECTATLANTIS_API UAtlantisInspectPartFocusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInspectPartFocusComponent();

	/**
	 * A part gets focus when, seen from the object's center, the angle between the part and the camera is below this.
	 * Smaller = the player has to face the part more squarely
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Parts",
		meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float PartFocusAngle = 0.f;

	/** Extra degrees a focused part keeps its focus for, so it doesn't flicker on and off right at the edge */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Parts",
		meta = (ClampMin = "0", ClampMax = "180", Units = "Degrees"))
	float PartFocusHysteresis = 0.f;

	/** Draws every part while inspecting: green = focused, yellow = focusable, grey = disabled. Not in shipping builds */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Debug")
	bool bDrawPartDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Debug",
		meta = (ClampMin = "0", Units = "Centimeters", EditCondition = "bDrawPartDebug"))
	float PartDebugRadius = 0.f;

	/** The part facing the camera right now, or null */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	UAtlantisInspectPartComponent* GetFocusedPart() const { return FocusedPart.Get(); }

	/**
	 * Where the focused part is on screen, in widget units (ready for a Canvas slot's position).
	 * False when no part is focused. Changes while rotating and zooming, so read it every frame
	 */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	bool GetFocusedPartScreenPosition(FVector2D& ScreenPosition) const;

	/** Focus moved to a part, between parts, or away (NewPart null, also when inspecting ends) */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|Inspection")
	FOnAtlantisPartFocusChanged OnPartFocusChanged;

	/** Only ticks while inspecting with bDrawPartDebug on (never in shipping), to draw the parts */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UFUNCTION()
	void HandleInspectStarted(UAtlantisInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleInspectEnded(UAtlantisInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleInspectRotated() { UpdatePartFocus(); }

	/** Focuses the enabled part most aligned with the camera, if within PartFocusAngle */
	void UpdatePartFocus();

	/** Changes the focus and broadcasts OnPartFocusChanged, if it actually changed */
	void SetFocusedPart(UAtlantisInspectPartComponent* NewPart);

	/** Forgets the parts and the focus */
	void ClearParts();

	/** Spheres on every part plus a line from the object's center, colored by focus. Every frame while bDrawPartDebug */
	void DrawPartDebug() const;

	/** How round the debug spheres are drawn. Looks only, debug builds only */
	static constexpr int32 PartDebugSphereSegments = 12;

	/** The Inspector we listen to, so we can unbind on EndPlay */
	TWeakObjectPtr<UAtlantisInspectorComponent> BoundInspector;

	/** The inspected object's parts, gathered on start */
	TArray<TWeakObjectPtr<UAtlantisInspectPartComponent>> Parts;

	TWeakObjectPtr<UAtlantisInspectPartComponent> FocusedPart;
};
