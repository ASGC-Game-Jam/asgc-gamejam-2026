// Copyright (c) 2026 ASGC

#include "Debug/AtlantisDebugMenuSettings.h"

UAtlantisDebugMenuSettings::UAtlantisDebugMenuSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("DebugMenu");
}

bool UAtlantisDebugMenuSettings::IsToggleKey(const FKey& Key) const
{
	return Key.IsValid() && (Key == ToggleMenuKey || Key == ToggleMenuGamepadKey);
}

bool UAtlantisDebugMenuSettings::IsResetKey(const FKey& Key) const
{
	return Key.IsValid() && (Key == ResetPlayerKey || Key == ResetPlayerGamepadKey);
}
