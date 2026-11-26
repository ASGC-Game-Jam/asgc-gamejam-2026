// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisInspectorComponent.generated.h"

class UAtlantisInspectableComponent;
class UEnhancedInputComponent;
class UInputAction;
class UInputMappingContext;
class APlayerController;
class UPrimitiveComponent;
struct FInputActionValue;

UCLASS(Blueprintable, ClassGroup=(Atlantis), meta=(DisplayName="Atlantis Inspector"))
class PROJECTATLANTIS_API UAtlantisInspectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisInspectorComponent();

	/** Mapping context added while inspecting */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Input")
	TObjectPtr<UInputMappingContext> InspectMappingContext;

	/** Closes the inspect screen */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Input")
	TObjectPtr<UInputAction> CloseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Input")
	TObjectPtr<UInputAction> RotateHoldAction;

	/** Only rotates while RotateHoldAction is held */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Input")
	TObjectPtr<UInputAction> RotateAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Input")
	TObjectPtr<UInputAction> ZoomAction;

	/** Degrees the object turns per unit of RotateAction */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Controls", meta = (ClampMin = "0"))
	float RotationSpeed = 0.f;

	/** How much one unit of ZoomAction changes the zoom scale */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Controls", meta = (ClampMin = "0"))
	float ZoomSpeed = 0.f;

	/** Closest zoom, as a multiplier of the framed distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Controls",
		meta = (ClampMin = "0.01", ClampMax = "1"))
	float MinZoomScale = 1.f;

	/** Farthest zoom, as a multiplier of the framed distance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Controls", meta = (ClampMin = "1"))
	float MaxZoomScale = 1.f;

	/** How much of the screen height the object fills (0-1). The distance to the camera is worked out from this and the object's size */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Framing",
		meta = (ClampMin = "0.01", ClampMax = "1"))
	float ScreenFillFraction = 0.f;

	/**
	 * Where the object's center sits on screen, as a fraction of half the screen: X right, Y up.
	 * (0,0) = screen center, (1,0) = right edge. Each object can add its own ScreenOffsetNudge
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|Inspection|Framing")
	FVector2D ScreenOffset = FVector2D::ZeroVector;

	/** Opens the inspect screen on this object, returns false on an invalid inspection */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Inspection")
	bool BeginInspect(UAtlantisInspectableComponent* Inspectable);

	/** Closes the inspect screen*/
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Inspection")
	void EndInspect();

	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	bool IsInspecting() const { return bInspecting; }

	/** The object being inspected, or null */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	UAtlantisInspectableComponent* GetInspectedObject() const { return InspectedObject.Get(); }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * Priority of InspectMappingContext. Above the gameplay contexts (0) that also use the mouse (look, UI click) so ours
	 * gets it first, below the debug menu (1000). 
	 */
	static constexpr int32 InspectMappingPriority = 10;

	APlayerController* GetPlayerController() const;

	/** Adds the inspect mapping context */
	void PushInspectInput(APlayerController* PlayerController);

	/** Undoes PushInspectInput */
	void PopInspectInput();

	void HandleCloseInput() { EndInspect(); }
	void HandleRotateHoldStarted() { bRotateHeld = true; }
	void HandleRotateHoldCompleted() { bRotateHeld = false; }
	void HandleRotate(const FInputActionValue& Value);
	void HandleZoom(const FInputActionValue& Value);

	/** Saves the object's state, works out the framing, then moves it in front of the camera with collision and physics off */
	void MoveObjectIntoView(AActor* Object, const UAtlantisInspectableComponent& Inspectable,
	                        const APlayerController& PlayerController);

	/** Places the object from the framing below plus the current ObjectRotation and ZoomScale */
	void ApplyObjectTransform();

	/** Puts the object back exactly as MoveObjectIntoView found it */
	void RestoreObject();

	/** Re-runs the owner's interaction trace, so the object loses its highlight once its collision is off */
	void RefreshInteractorFocus() const;

	/** What MoveObjectIntoView changed, so RestoreObject can undo it */
	TWeakObjectPtr<AActor> MovedObject;
	FTransform OriginalTransform;
	bool bOriginalCollisionEnabled = true;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> PausedPhysicsComponents;

	/**
	 * Slows the world down to the engine's minimum time dilation instead of pausing it
	 */
	void FreezeWorld();

	/** Puts the world's time dilation back to what it was before FreezeWorld */
	void UnfreezeWorld();

	/** Global time dilation before FreezeWorld */
	float SavedTimeDilation = 1.f;

	/** Framing worked out once on open. The camera can't move while the world is frozen */
	FVector ViewLocation = FVector::ZeroVector;
	FQuat ViewRotation = FQuat::Identity;
	float TanHalfHorizontalFOV = 0.f;
	float TanHalfVerticalFOV = 0.f;
	FVector2D FramedScreenOffset = FVector2D::ZeroVector;
	float FramedDistance = 0.f;
	/** The object's bounds center relative to its pivot, scaled but not rotated. Used to rotate around the center */
	FVector ScaledLocalBoundsCenter = FVector::ZeroVector;

	/** What the player changes while inspecting */
	FQuat ObjectRotation = FQuat::Identity;
	float ZoomScale = 1.f;
	bool bRotateHeld = false;

	TWeakObjectPtr<UAtlantisInspectableComponent> InspectedObject;

	/** Separate from InspectedObject so we can still clean up if the object gets destroyed mid-inspect */
	bool bInspecting = false;

	UPROPERTY(Transient)
	TObjectPtr<UEnhancedInputComponent> InspectInputComponent;

	TWeakObjectPtr<APlayerController> InputPlayerController;
};
