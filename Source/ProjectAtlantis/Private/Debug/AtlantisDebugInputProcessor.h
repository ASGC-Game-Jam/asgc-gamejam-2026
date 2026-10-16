// Copyright (c) 2026 ASGC

#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "Debug/AtlantisDebugInputState.h"

class AAtlantisPlayerController;

/** Intercepts debug controls before CommonUI and Enhanced Input can also handle them. */
class FAtlantisDebugInputProcessor : public IInputProcessor
{
public:
	explicit FAtlantisDebugInputProcessor(AAtlantisPlayerController* InController);
	virtual void Tick(float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override;

private:
	bool IsOwningUser(const FKeyEvent& Event) const;
	bool IsGameViewportFocused(FSlateApplication& SlateApp, const FKeyEvent& Event) const;
	TWeakObjectPtr<AAtlantisPlayerController> Controller;
	FAtlantisDebugInputState State;
};
