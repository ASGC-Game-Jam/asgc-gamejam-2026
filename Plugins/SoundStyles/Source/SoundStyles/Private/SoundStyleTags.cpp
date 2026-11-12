// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleTags.h"

// Roots
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Class, "UISound.Class", "Root of all UI sound classes. Its row in the style table is the game-wide default.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event, "UISound.Event", "Root of all UI sound events. Games add their own events under this.");

// Events the plugin plays itself
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Hover, "UISound.Event.Hover", "Mouse over, or gamepad/keyboard focus moved onto the widget.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Press, "UISound.Event.Press", "Button pressed down.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Release, "UISound.Event.Release", "Button released.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Open, "UISound.Event.Open", "A screen opened.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Close, "UISound.Event.Close", "A screen closed.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Hold, "UISound.Event.Hold", "Plays while a widget is held (button pressed, slider dragged); stops when let go. Use a looping sound.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Step, "UISound.Event.Step", "Plays each time a slider or progress bar crosses one of its steps (detents). Sent with Value (0-1).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SoundStyles_Event_Fill, "UISound.Event.Fill", "Plays while a progress bar is moving; stops shortly after it stops. Use a looping sound. Sent with Value (0-1).");
