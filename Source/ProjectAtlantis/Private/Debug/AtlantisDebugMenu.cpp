// Copyright (c) 2026 ASGC

#include "Debug/AtlantisDebugMenu.h"
#include "Debug/AtlantisCheatManager.h"
#include "Core/AtlantisPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Engine/LocalPlayer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UAtlantisDebugMenu::RebuildWidget()
{
#if !UE_BUILD_SHIPPING
	TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);
	Entries->AddSlot().AutoHeight().Padding(8.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("AtlantisDebug", "Title", "Development Debug Menu"))
	];
	AAtlantisPlayerController* Controller = Cast<AAtlantisPlayerController>(GetOwningPlayer());
	UAtlantisCheatManager* Cheats = Controller ? Cast<UAtlantisCheatManager>(Controller->CheatManager) : nullptr;
	if (Cheats)
	{
		for (const FAtlantisDebugAction& Action : Cheats->GetDebugActions())
		{
			const TWeakObjectPtr<AAtlantisPlayerController> WeakController(Controller);
			TSharedRef<SButton> Button = SNew(SButton)
				.Text(Action.Label)
				.OnClicked_Lambda([WeakController, Command = Action.Command]()
				{
					if (AAtlantisPlayerController* Player = WeakController.Get())
					{
						Player->ConsoleCommand(Command, false);
					}
					return FReply::Handled();
				});
			if (!FirstAction)
			{
				FirstAction = Button;
			}
			Entries->AddSlot().AutoHeight().Padding(8.f)[Button];
		}
	}
	Entries->AddSlot().AutoHeight().Padding(8.f)
	[
		SNew(SButton)
		.Text(NSLOCTEXT("AtlantisDebug", "Close", "Close (F7 / View / B)"))
		.OnClicked_Lambda([this]()
		{
			if (AAtlantisPlayerController* Player = Cast<AAtlantisPlayerController>(GetOwningPlayer()))
			{
				Player->ToggleDebugMenu();
			}
			return FReply::Handled();
		})
	];
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
	[
		SNew(SBox).WidthOverride(420.f).MaxDesiredHeight(600.f)
		[
			SNew(SBorder).Padding(16.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()[Entries]
			]
		]
	];
#else
	return SNullWidget::NullWidget;
#endif
}

void UAtlantisDebugMenu::FocusFirstAction()
{
	if (FirstAction && GetOwningPlayer() && GetOwningPlayer()->GetLocalPlayer())
	{
		if (const TSharedPtr<FSlateUser> User = GetOwningPlayer()->GetLocalPlayer()->GetSlateUser())
		{
			FSlateApplication::Get().SetUserFocus(User->GetUserIndex(), FirstAction, EFocusCause::SetDirectly);
		}
	}
}

void UAtlantisDebugMenu::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	FirstAction.Reset();
}

FReply UAtlantisDebugMenu::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	AAtlantisPlayerController* Player = Cast<AAtlantisPlayerController>(GetOwningPlayer());
	const FKey Key = Event.GetKey();
	if (Player && (Key == EKeys::F7 || Key == EKeys::Escape ||
		Key == EKeys::Gamepad_Special_Left || Key == EKeys::Gamepad_FaceButton_Right))
	{
		Player->ToggleDebugMenu();
		return FReply::Handled();
	}
	if (Player && (Key == EKeys::F10 || Key == EKeys::Gamepad_RightThumbstick))
	{
		Player->RequestDebugReset();
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}
