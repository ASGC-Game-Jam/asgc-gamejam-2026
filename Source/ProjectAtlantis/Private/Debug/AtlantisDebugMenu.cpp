// Copyright (c) 2026 ASGC

#include "Debug/AtlantisDebugMenu.h"
#include "Debug/AtlantisCheatManager.h"
#include "Debug/AtlantisDebugMenuSettings.h"
#include "Core/AtlantisPlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Engine/LocalPlayer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

UAtlantisDebugMenu::UAtlantisDebugMenu(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// UI-only input first focuses our SObjectWidget before selecting the initial button.
	SetIsFocusable(true);
}

TSharedRef<SWidget> UAtlantisDebugMenu::RebuildWidget()
{
#if !UE_BUILD_SHIPPING
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 32);
	const FSlateFontInfo ActionFont = FCoreStyle::GetDefaultFontStyle("Regular", 24);
	const UAtlantisDebugMenuSettings* Settings = GetDefault<UAtlantisDebugMenuSettings>();
	const FText GamepadToggleHint = Settings->ToggleMenuGamepadModifier.IsValid()
		? FText::Format(NSLOCTEXT("AtlantisDebug", "GamepadToggleChord", "{0} + {1}"),
			Settings->ToggleMenuGamepadModifier.GetDisplayName(), Settings->ToggleMenuGamepadKey.GetDisplayName())
		: Settings->ToggleMenuGamepadKey.GetDisplayName();
	TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);
	Entries->AddSlot().AutoHeight().Padding(12.f, 8.f, 12.f, 16.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("AtlantisDebug", "Title", "Development Debug Menu"))
		.Font(TitleFont)
	];
	Entries->AddSlot().AutoHeight().Padding(12.f, 0.f, 12.f, 20.f)
	[
		SNew(STextBlock)
		.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
		.AutoWrapText(true)
		.Text(FText::Format(NSLOCTEXT("AtlantisDebug", "Shortcuts",
			"Menu: {0} / {1}\nReset: {2} / {3} (in menu)"),
			Settings->ToggleMenuKey.GetDisplayName(), GamepadToggleHint,
			Settings->ResetPlayerKey.GetDisplayName(), Settings->ResetPlayerGamepadKey.GetDisplayName()))
	];
	AAtlantisPlayerController* Controller = Cast<AAtlantisPlayerController>(GetOwningPlayer());
	UAtlantisCheatManager* Cheats = Controller ? Cast<UAtlantisCheatManager>(Controller->CheatManager) : nullptr;
	if (Cheats)
	{
		for (const FAtlantisDebugAction& Action : Cheats->GetDebugActions())
		{
			const TWeakObjectPtr<AAtlantisPlayerController> WeakController(Controller);
			TSharedRef<SButton> Button = SNew(SButton)
				.ContentPadding(FMargin(24.f, 18.f))
				.HAlign(HAlign_Center)
				.OnClicked_Lambda([WeakController, Command = Action.Command]()
				{
					if (AAtlantisPlayerController* Player = WeakController.Get())
					{
						Player->ConsoleCommand(Command, false);
					}
					return FReply::Handled();
				})
				[
					SNew(STextBlock).Text(Action.Label).Font(ActionFont)
				];
			if (!FirstAction)
			{
				FirstAction = Button;
			}
			Entries->AddSlot().AutoHeight().Padding(12.f, 8.f)[Button];
		}
	}
	Entries->AddSlot().AutoHeight().Padding(12.f, 8.f)
	[
		SNew(SButton)
		.ContentPadding(FMargin(24.f, 18.f))
		.HAlign(HAlign_Center)
		.OnClicked_Lambda([this]()
		{
			if (AAtlantisPlayerController* Player = Cast<AAtlantisPlayerController>(GetOwningPlayer()))
			{
				Player->ToggleDebugMenu();
			}
			return FReply::Handled();
		})
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("AtlantisDebug", "Close", "Close (Esc / B)"))
			.Font(ActionFont)
		]
	];
	return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center).Padding(24.f)
	[
		SNew(SScaleBox).Stretch(EStretch::ScaleToFit).StretchDirection(EStretchDirection::DownOnly)
		[
			SNew(SBox).WidthOverride(720.f).MinDesiredHeight(380.f).MaxDesiredHeight(720.f)
			[
				SNew(SBorder).Padding(24.f)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FLinearColor(0.025f, 0.035f, 0.05f, 1.f))
				.ForegroundColor(FLinearColor::White)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()[Entries]
				]
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
