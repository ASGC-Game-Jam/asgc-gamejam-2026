// Copyright (c) 2026 ASGC

#include "UI/AtlantisBallastIndicatorWidget.h"

#include "CommonInputSubsystem.h"
#include "Components/TextBlock.h"

#define LOCTEXT_NAMESPACE "AtlantisBallastIndicator"

void UAtlantisBallastIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InputSubsystem = UCommonInputSubsystem::Get(GetOwningLocalPlayer());
	if (UCommonInputSubsystem* Subsystem = InputSubsystem.Get())
	{
		InputMethodChangedHandle = Subsystem->OnInputMethodChangedNative.AddUObject(
			this, &UAtlantisBallastIndicatorWidget::UpdateInputHints);
		UpdateInputHints(Subsystem->GetCurrentInputType());
	}
	else
	{
		UpdateInputHints(ECommonInputType::MouseAndKeyboard);
	}
}

void UAtlantisBallastIndicatorWidget::NativeDestruct()
{
	if (UCommonInputSubsystem* Subsystem = InputSubsystem.Get())
	{
		Subsystem->OnInputMethodChangedNative.Remove(InputMethodChangedHandle);
	}
	InputMethodChangedHandle.Reset();
	InputSubsystem.Reset();
	Super::NativeDestruct();
}

void UAtlantisBallastIndicatorWidget::UpdateInputHints(ECommonInputType InputType)
{
	if (UTextBlock* Hints = Cast<UTextBlock>(GetWidgetFromName(TEXT("BallastStateKeys"))))
	{
		Hints->SetText(InputType == ECommonInputType::Gamepad
			? LOCTEXT("GamepadHints", "Descend: B | Wander: X | Ascend: Y")
			: LOCTEXT("KeyboardHints", "Descend: 2 | Wander: 3 | Ascend: 4"));
	}
}

#undef LOCTEXT_NAMESPACE