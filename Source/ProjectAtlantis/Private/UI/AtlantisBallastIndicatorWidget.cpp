// Copyright (c) 2026 ASGC

#include "UI/AtlantisBallastIndicatorWidget.h"

#include "Components/TextBlock.h"
#include "Core/AtlantisPlayerController.h"

#define LOCTEXT_NAMESPACE "AtlantisBallastIndicator"

void UAtlantisBallastIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InputController = Cast<AAtlantisPlayerController>(GetOwningPlayer());
	if (AAtlantisPlayerController* Controller = InputController.Get())
	{
		Controller->OnInputDeviceChanged.AddUniqueDynamic(this, &UAtlantisBallastIndicatorWidget::UpdateInputHints);
	}
	UpdateInputHints(InputController.IsValid() && InputController->IsUsingGamepad());
}

void UAtlantisBallastIndicatorWidget::NativeDestruct()
{
	if (AAtlantisPlayerController* Controller = InputController.Get())
	{
		Controller->OnInputDeviceChanged.RemoveDynamic(this, &UAtlantisBallastIndicatorWidget::UpdateInputHints);
	}
	InputController.Reset();
	Super::NativeDestruct();
}

void UAtlantisBallastIndicatorWidget::UpdateInputHints(bool bUsingGamepad)
{
	if (UTextBlock* Hints = Cast<UTextBlock>(GetWidgetFromName(TEXT("BallastStateKeys"))))
	{
		Hints->SetText(bUsingGamepad
			? LOCTEXT("GamepadHints", "Descend: B | Wander: X | Ascend: Y")
			: LOCTEXT("KeyboardHints", "Descend: 2 | Wander: 3 | Ascend: 4"));
	}
}

#undef LOCTEXT_NAMESPACE