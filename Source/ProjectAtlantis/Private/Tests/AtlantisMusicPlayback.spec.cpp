// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Engine/World.h"
#include "MetasoundSource.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(FAtlantisMusicPlaybackSpec, "Atlantis.Audio.MusicPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UWorld> World;
END_DEFINE_SPEC(FAtlantisMusicPlaybackSpec)

void FAtlantisMusicPlaybackSpec::Define()
{
	AfterEach([this]()
	{
		World.Reset();
	});

	Describe("Gameplay map selection", [this]()
	{
		It("should enable music in the tutorial and district entrance", [this]()
		{
			for (const TCHAR* Map : {
				TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_Main"),
				TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_District_Entrance")})
			{
				World.Reset(NewObject<UWorld>(CreatePackage(Map), NAME_None, RF_Transient));
				World->WorldType = EWorldType::Game;
				TestTrue(Map, UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			}
		});

		It("should recognize a PIE copy of the tutorial map", [this]()
		{
			World.Reset(NewObject<UWorld>(CreatePackage(TEXT("/Game/ProjectAtlantis/Maps/Tutorial/UEDPIE_2_L_Tutorial_Main")), NAME_None, RF_Transient));
			World->WorldType = EWorldType::PIE;
			TestTrue(TEXT("PIE tutorial has music"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
		});

		It("should leave main menu music to its existing owner", [this]()
		{
			World.Reset(NewObject<UWorld>(CreatePackage(TEXT("/Game/ProjectAtlantis/Maps/L_MainMenu")), NAME_None, RF_Transient));
			World->WorldType = EWorldType::Game;
			TestFalse(TEXT("No subsystem music in menu"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
		});

		It("should not start music in editor worlds or unconfigured maps", [this]()
		{
			World.Reset(NewObject<UWorld>(CreatePackage(TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_Main")), NAME_None, RF_Transient));
			World->WorldType = EWorldType::Editor;
			TestFalse(TEXT("No editor playback"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			World.Reset(NewObject<UWorld>(CreatePackage(TEXT("/Game/Tests/UnconfiguredMusicMap")), NAME_None, RF_Transient));
			World->WorldType = EWorldType::Game;
			TestFalse(TEXT("No unconfigured map playback"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			TestFalse(TEXT("No null world playback"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(nullptr));
		});
	});

	Describe("Configured content", [this]()
	{
		It("should load all three stereo playlist tracks from the music folder", [this]()
		{
			for (const TCHAR* Name : { TEXT("Ambient"), TEXT("Passive"), TEXT("Active") })
			{
				const FString Path = FString::Printf(TEXT("/Game/ProjectAtlantis/Audio/Music/SW_SteamCore_%s_V1"), Name);
				const USoundWave* Track = LoadObject<USoundWave>(nullptr, *Path);
				if (TestNotNull(*Path, Track))
				{
					TestEqual(TEXT("Stereo track"), Track->NumChannels, 2);
					TestTrue(TEXT("Track has audio"), Track->Duration > 0.f);
				}
			}
		});

		It("should load a stereo music source with music mixing routes", [this]()
		{
			UMetaSoundSource* Music = UAtlantisAudioDeveloperSettings::GetMusicMetaSoundSource();
			if (TestNotNull(TEXT("Configured music loads"), Music))
			{
				TestEqual(TEXT("Stereo output"), Music->NumChannels, 2);
				TestNotNull(TEXT("Music sound class"), Music->SoundClassObject.Get());
				TestNotNull(TEXT("Music submix"), Music->SoundSubmixObject.Get());
			}
		});
	});
}

#endif
