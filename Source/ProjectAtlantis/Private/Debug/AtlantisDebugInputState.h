// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Debug/AtlantisDebugMenuSettings.h"

enum class EAtlantisDebugInputAction : uint8
{
	None,
	ToggleMenu,
	ResetPlayer
};

struct FAtlantisDebugInputResult
{
	EAtlantisDebugInputAction Action = EAtlantisDebugInputAction::None;
	bool bHandled = false;
};

/** Tracks complete button presses, including the release after a menu closes. */
class FAtlantisDebugInputState
{
public:
	FAtlantisDebugInputResult KeyDown(const FKey& Key, bool bRepeat,
		const UAtlantisDebugMenuSettings& Settings, bool bMenuOpen)
	{
		PressedKeys.Add(Key);
		if (CapturedKeys.Contains(Key))
		{
			return {EAtlantisDebugInputAction::None, true};
		}
		if (bRepeat || !Key.IsValid())
		{
			return {};
		}

		const bool bToggle = Settings.IsToggleKey(Key)
			&& (bMenuOpen || Key != Settings.ToggleMenuGamepadKey
				|| !Settings.ToggleMenuGamepadModifier.IsValid()
				|| PressedKeys.Contains(Settings.ToggleMenuGamepadModifier));
		const bool bClose = bMenuOpen && (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right);
		if (bToggle || bClose)
		{
			CapturedKeys.Add(Key);
			return {EAtlantisDebugInputAction::ToggleMenu, true};
		}
		if (Settings.IsResetKey(Key) && (!Key.IsGamepadKey() || bMenuOpen))
		{
			CapturedKeys.Add(Key);
			return {EAtlantisDebugInputAction::ResetPlayer, true};
		}
		return {};
	}

	bool KeyUp(const FKey& Key)
	{
		PressedKeys.Remove(Key);
		return CapturedKeys.Remove(Key) > 0;
	}

	void Reset()
	{
		PressedKeys.Reset();
		CapturedKeys.Reset();
	}

private:
	TSet<FKey> PressedKeys;
	TSet<FKey> CapturedKeys;
};
