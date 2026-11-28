// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisInspectWidget.generated.h"

class UAtlantisInspectableComponent;
class UAtlantisInspectorComponent;
class UAtlantisInspectPartComponent;
class UAtlantisInspectPartFocusComponent;

UCLASS(Abstract, Blueprintable)
class PROJECTATLANTIS_API UAtlantisInspectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Called by the Presentation after the widget is on screen */
	void StartInspect(UAtlantisInspectorComponent* InInspector, UAtlantisInspectableComponent* Inspectable);

	/** Called by the Presentation before the widget is removed */
	void EndInspect(UAtlantisInspectableComponent* Inspectable);

	/** The Inspector that opened this screen*/
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	UAtlantisInspectorComponent* GetInspector() const { return Inspector.Get(); }

	/** Where the focused part is on screen, for a Canvas slot's position. False when no part is focused */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Inspection")
	bool GetFocusedPartScreenPosition(FVector2D& ScreenPosition) const;

protected:
	/** Show the prompt when Part is set, hide it when null. Also called on start if a part already faces the camera */
	UFUNCTION(BlueprintImplementableEvent, Category = "Atlantis|Inspection")
	void OnPartFocusChanged(UAtlantisInspectPartComponent* Part);

	/** Fill the screen here: name, description... from the Inspectable */
	UFUNCTION(BlueprintImplementableEvent, Category = "Atlantis|Inspection")
	void OnInspectStarted(UAtlantisInspectableComponent* Inspectable);

	/** The widget is removed right after this. Inspectable can be null if the object was destroyed mid-inspect */
	UFUNCTION(BlueprintImplementableEvent, Category = "Atlantis|Inspection")
	void OnInspectEnded(UAtlantisInspectableComponent* Inspectable);

private:
	UFUNCTION()
	void HandlePartFocusChanged(UAtlantisInspectPartComponent* NewPart, UAtlantisInspectPartComponent* OldPart);

	TWeakObjectPtr<UAtlantisInspectorComponent> Inspector;

	/** The player's part focus (next to the Inspector), if it has one */
	TWeakObjectPtr<UAtlantisInspectPartFocusComponent> PartFocus;
};
