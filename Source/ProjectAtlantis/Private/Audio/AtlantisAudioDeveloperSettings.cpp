// Copyright (c) 2026 ASGC


#include "Audio/AtlantisAudioDeveloperSettings.h"

#include "HAL/IConsoleManager.h"
#include "StructUtils/InstancedStruct.h"
#include "MetasoundSource.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioValidation, Log, All);

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

TObjectPtr<UMetaSoundSource> UAtlantisAudioDeveloperSettings::GetMusicMetaSoundSource()
{
	const UAtlantisAudioDeveloperSettings* Settings = GetDefault<UAtlantisAudioDeveloperSettings>();
	if (!Settings)
	{
		return nullptr;
	}

	if (Settings->MusicMetaSoundSource.IsNull())
	{
		return nullptr;
	}

	if (Settings->MusicMetaSoundSource.IsValid())
	{
		return Settings->MusicMetaSoundSource.Get();
	}

	return Settings->MusicMetaSoundSource.LoadSynchronous();
}

bool UAtlantisAudioDeveloperSettings::ValidatePayload(FGameplayTag Tag, const FInstancedStruct& Payload)
{
	const UAtlantisAudioDeveloperSettings* Settings = GetDefault<UAtlantisAudioDeveloperSettings>();
	if (!Settings)
	{
		return false;
	}

	// Check 1: Is there an address on the envelope at all?
	if (!Tag.IsValid())
	{
		UE_LOG(LogAtlantisAudioValidation, Error, TEXT("Audio payload sent with an empty/invalid GameplayTag. Every audio fact needs a registered tag."));
		return false;
	}

	// Check 2: Is this address on the list? (No row for this tag means Find returns nullptr.)
	const TSoftObjectPtr<const UScriptStruct>* RegisteredStruct = Settings->PayloadRegistry.Find(Tag);
	if (!RegisteredStruct)
	{
		UE_LOG(LogAtlantisAudioValidation, Error, TEXT("Audio tag [%s] is not registered. Add it in Project Settings > Atlantis Audio Validation."), *Tag.ToString());
		return false;
	}

	// Check 3: The row exists, but does its struct actually resolve? (Blank dropdown, or the struct was renamed/deleted.)
	const UScriptStruct* ExpectedStruct = RegisteredStruct->LoadSynchronous();
	if (!ExpectedStruct)
	{
		UE_LOG(LogAtlantisAudioValidation, Error, TEXT("Audio tag [%s] is registered but its struct is empty or missing. Pick one in Project Settings > Atlantis Audio Validation."), *Tag.ToString());
		return false;
	}

	// Check 4: Is the package the right shape for this address?
	const UScriptStruct* PayloadStruct = Payload.GetScriptStruct();
	if (PayloadStruct != ExpectedStruct)
	{
		UE_LOG(LogAtlantisAudioValidation, Error, TEXT("Audio payload for tag [%s] holds struct [%s] but the registry expects [%s]."), *Tag.ToString(), *GetNameSafe(PayloadStruct), *GetNameSafe(ExpectedStruct));
		return false;
	}

	// All four checks passed, so it's safe to route.
	return true;

}