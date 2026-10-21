// Copyright (c) 2026 ASGC


#include "Audio/AtlantisAudioDeveloperSettings.h"

#include "HAL/IConsoleManager.h"


static TAutoConsoleVariable<int32> CVarAtlantisAudioMusicSubsystemState(
	TEXT("atlantis.audio.musicSubsystem"),
	0,
	TEXT("Determines the state of the Atlantis Music Subsystem\n")
	TEXT("  0: Use the checkbox (default)\n")
	TEXT("  1: Force off\n")
	TEXT("  2: Force on \n")
);


FName UAtlantisAudioDeveloperSettings::GetCategoryName() const
{
	return TEXT("Game");
}

#if WITH_EDITOR
FText UAtlantisAudioDeveloperSettings::GetSectionText() const
{
	return NSLOCTEXT("AtlantisAudioSettingsAdvanced", "AtlantisAudioSettings", "Atlantis Audio Settings");
}

FText UAtlantisAudioDeveloperSettings::GetSectionDescription() const
{
	return NSLOCTEXT("AtlantisAudioSettingsAdvanced", "AtlantisAudioSettingsDescription", "All Atlantis audio dev settings.");
}
#endif


bool UAtlantisAudioDeveloperSettings::DetermineIfMusicSubsystemEnabled()
{
	int32 cVarState = CVarAtlantisAudioMusicSubsystemState.GetValueOnGameThread();
	const UAtlantisAudioDeveloperSettings* Settings = GetDefault<UAtlantisAudioDeveloperSettings>();

	if (!Settings)
	{
		return false;
	}

	bool ProjectSettingsSelection = Settings->bEnableMusicManagementSubsystem;

	switch(cVarState)
	{
		case 0:
		default:
			return ProjectSettingsSelection;
		case 1:
			return false;
		case 2:
			return true;
	}
}