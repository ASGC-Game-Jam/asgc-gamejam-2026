// Copyright (c) 2026 ASGC

#include "Debug/AtlantisDebugInputProcessor.h"
#include "Core/AtlantisPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

FAtlantisDebugInputProcessor::FAtlantisDebugInputProcessor(AAtlantisPlayerController* InController)
	: Controller(InController)
{
}

void FAtlantisDebugInputProcessor::Tick(float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	if (!SlateApp.IsActive() || !Controller.IsValid())
	{
		State.Reset();
	}
}

bool FAtlantisDebugInputProcessor::IsOwningUser(const FKeyEvent& Event) const
{
	const AAtlantisPlayerController* Player = Controller.Get();
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	const TSharedPtr<const FSlateUser> User = LocalPlayer ? LocalPlayer->GetSlateUser() : nullptr;
	return User && static_cast<uint32>(User->GetUserIndex()) == Event.GetUserIndex();
}

bool FAtlantisDebugInputProcessor::IsGameViewportFocused(FSlateApplication& SlateApp, const FKeyEvent& Event) const
{
	const AAtlantisPlayerController* Player = Controller.Get();
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	const UGameViewportClient* Viewport = LocalPlayer ? LocalPlayer->ViewportClient.Get() : nullptr;
	const TSharedPtr<SViewport> ViewportWidget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
	const TSharedPtr<SWidget> FocusedWidget = SlateApp.GetUserFocusedWidget(Event.GetUserIndex());
	if (!ViewportWidget || !FocusedWidget ||
		SlateApp.FindWidgetWindow(ViewportWidget.ToSharedRef()) != SlateApp.FindWidgetWindow(FocusedWidget.ToSharedRef()))
	{
		return false;
	}
	// Match screenshot capture's PIE scoping without stealing keys from editor panels.
	const FGeometry& Geometry = FocusedWidget->GetCachedGeometry();
	const FVector2D Center = Geometry.LocalToAbsolute(Geometry.GetLocalSize() * 0.5f);
	return ViewportWidget->HasAnyUserFocusOrFocusedDescendants()
		|| ViewportWidget->GetCachedGeometry().IsUnderLocation(Center);
}

bool FAtlantisDebugInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event)
{
#if !UE_BUILD_SHIPPING
	if (!IsOwningUser(Event) || !IsGameViewportFocused(SlateApp, Event))
	{
		return false;
	}
	AAtlantisPlayerController* Player = Controller.Get();
	const FAtlantisDebugInputResult Result = State.KeyDown(Event.GetKey(), Event.IsRepeat(),
		*GetDefault<UAtlantisDebugMenuSettings>(), Player->IsDebugMenuOpen());
	switch (Result.Action)
	{
	case EAtlantisDebugInputAction::ToggleMenu:
		Player->ToggleDebugMenu();
		break;
	case EAtlantisDebugInputAction::ResetPlayer:
		Player->RequestDebugReset();
		break;
	default:
		break;
	}
	return Result.bHandled;
#else
	return false;
#endif
}

bool FAtlantisDebugInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& Event)
{
	// Consume the release even if closing the menu already restored gameplay focus.
	return IsOwningUser(Event) && State.KeyUp(Event.GetKey());
}
