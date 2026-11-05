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
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateImageBrush.h"
#include "CommonInputSubsystem.h"
#include "CommonInputBaseTypes.h"
#include "Misc/Paths.h"

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
	const FSlateFontInfo HintFont = FCoreStyle::GetDefaultFontStyle("Regular", 18);
	auto HintText = [&HintFont](const FText& Text)
	{
		return SNew(STextBlock).Text(Text).Font(HintFont);
	};
	TSharedRef<SHorizontalBox> MenuHint = SNew(SHorizontalBox);
	MenuHint->AddSlot().AutoWidth().VAlign(VAlign_Center)
	[
		HintText(FText::Format(NSLOCTEXT("AtlantisDebug", "MenuKeyboardHint", "Menu: {0} / "),
			Settings->ToggleMenuKey.GetDisplayName()))
	];
	if (Settings->ToggleMenuGamepadKey.IsValid() && Settings->ToggleMenuGamepadModifier.IsValid())
	{
		MenuHint->AddSlot().AutoWidth().VAlign(VAlign_Center)[MakeKeyHint(Settings->ToggleMenuGamepadModifier)];
		MenuHint->AddSlot().AutoWidth().VAlign(VAlign_Center)[HintText(FText::FromString(TEXT(" + ")))];
	}
	MenuHint->AddSlot().AutoWidth().VAlign(VAlign_Center)[MakeKeyHint(Settings->ToggleMenuGamepadKey)];
	TSharedRef<SHorizontalBox> ResetHint = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			HintText(FText::Format(NSLOCTEXT("AtlantisDebug", "RestartKeyboardHint", "Restart: {0} / "),
				Settings->ResetPlayerKey.GetDisplayName()))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[MakeKeyHint(Settings->ResetPlayerGamepadKey)]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			HintText(NSLOCTEXT("AtlantisDebug", "InMenuHint", " (in menu)"))
		];
	TSharedRef<SVerticalBox> Entries = SNew(SVerticalBox);
	Entries->AddSlot().AutoHeight().Padding(12.f, 8.f, 12.f, 16.f)
	[
		SNew(STextBlock).Text(NSLOCTEXT("AtlantisDebug", "Title", "Debug Menu"))
		.Font(TitleFont)
		.Justification(ETextJustify::Center)
	];
	Entries->AddSlot().AutoHeight().Padding(12.f, 0.f, 12.f, 20.f)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[MenuHint]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)[ResetHint]
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
	InputIconBrushes.Reset();
}

TSharedRef<SWidget> UAtlantisDebugMenu::MakeKeyHint(const FKey& Key)
{
	TSharedPtr<FSlateBrush> Brush = MakeShared<FSlateBrush>();
	const UCommonInputSubsystem* Input = UCommonInputSubsystem::Get(GetOwningLocalPlayer());
	const FName GamepadName = Input ? Input->GetCurrentGamepadName() : FCommonInputDefaults::GamepadGeneric;
	const bool bHasControllerIcon = Key.IsGamepadKey()
		&& UCommonInputPlatformSettings::Get()->TryGetInputBrush(*Brush, Key, ECommonInputType::Gamepad, GamepadName);
	if (!bHasControllerIcon)
	{
		const TCHAR* IconFile = nullptr;
		if (Key == EKeys::Gamepad_LeftThumbstick)
		{
			IconFile = TEXT("Icon_Gamepad_LeftStickPress.svg");
		}
		else if (Key == EKeys::Gamepad_RightThumbstick)
		{
			IconFile = TEXT("Icon_Gamepad_RightStickPress.svg");
		}
		else if (Key == EKeys::Gamepad_Special_Right)
		{
			IconFile = TEXT("Icon_Gamepad_Menu.svg");
		}
		if (!IconFile)
		{
			return SNew(STextBlock).Text(Key.GetDisplayName())
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18));
		}
		Brush = MakeShared<FSlateVectorImageBrush>(
			FPaths::ProjectContentDir() / TEXT("ProjectAtlantis/UI/Debug/Icons") / IconFile, FVector2D(36.f, 36.f));
	}
	InputIconBrushes.Add(Brush);
	return SNew(SBox).WidthOverride(36.f).HeightOverride(36.f).ToolTipText(Key.GetDisplayName())
	[
		SNew(SImage).Image(Brush.Get())
	];
}
