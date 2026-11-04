// Copyright (c) 2026 ASGC

#include "Debug/AtlantisDebugMenuSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"

UAtlantisDebugMenuSettings::UAtlantisDebugMenuSettings()
{
	CategoryName = TEXT("Game");
	SectionName = TEXT("DebugMenu");
}

FAtlantisDebugInputMappings UAtlantisDebugMenuSettings::CreateInputMappings(UObject* Outer) const
{
	FAtlantisDebugInputMappings Mappings;
#if !UE_BUILD_SHIPPING
	Mappings.Gameplay = NewObject<UInputMappingContext>(Outer, TEXT("IMC_DebugGameplay"), RF_Transient);
	Mappings.Menu = NewObject<UInputMappingContext>(Outer, TEXT("IMC_DebugMenu"), RF_Transient);
	auto PressAction = [Outer](const FName Name)
	{
		UInputAction* Action = NewObject<UInputAction>(Outer, Name, RF_Transient);
		Action->ValueType = EInputActionValueType::Boolean;
		Action->Triggers.Add(NewObject<UInputTriggerPressed>(Action));
		return Action;
	};
	Mappings.Toggle = PressAction(TEXT("IA_DebugToggle"));
	Mappings.GamepadToggle = PressAction(TEXT("IA_DebugGamepadToggle"));
	Mappings.Reset = PressAction(TEXT("IA_DebugReset"));
	Mappings.Close = PressAction(TEXT("IA_DebugClose"));
	auto MapKey = [](UInputMappingContext* Context, UInputAction* Action, const FKey& Key)
	{
		if (Key.IsValid())
		{
			Context->MapKey(Action, Key);
		}
	};
	MapKey(Mappings.Gameplay, Mappings.Toggle, ToggleMenuKey);
	MapKey(Mappings.Gameplay, Mappings.Reset, ResetPlayerKey);
	if (ToggleMenuGamepadKey.IsValid())
	{
		UInputTriggerChordAction* Chord = nullptr;
		if (ToggleMenuGamepadModifier.IsValid())
		{
			UInputAction* Modifier = NewObject<UInputAction>(Outer, TEXT("IA_DebugModifier"), RF_Transient);
			Modifier->bConsumeInput = false;
			Mappings.Gameplay->MapKey(Modifier, ToggleMenuGamepadModifier);
			Chord = NewObject<UInputTriggerChordAction>(Mappings.Gameplay);
			Chord->ChordAction = Modifier;
		}
		FEnhancedActionKeyMapping& ToggleMapping = Mappings.Gameplay->MapKey(Mappings.GamepadToggle, ToggleMenuGamepadKey);
		if (Chord)
		{
			ToggleMapping.Triggers.Add(Chord);
		}
	}

	MapKey(Mappings.Menu, Mappings.Toggle, ToggleMenuKey);
	MapKey(Mappings.Menu, Mappings.GamepadToggle, ToggleMenuGamepadKey);
	MapKey(Mappings.Menu, Mappings.Close, EKeys::Escape);
	MapKey(Mappings.Menu, Mappings.Close, EKeys::Gamepad_FaceButton_Right);
	MapKey(Mappings.Menu, Mappings.Reset, ResetPlayerKey);
	MapKey(Mappings.Menu, Mappings.Reset, ResetPlayerGamepadKey);
#endif
	return Mappings;
}
