// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Engine/World.h"
#include "MetasoundSource.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"
#include "Sound/SoundWave.h"
#include "Tests/AtlantisAudioTestAccess.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	// Two real MetaSound Sources, so the per-map tests can tell scores apart.
	const TCHAR* ScoreAPath = TEXT("/Game/ProjectAtlantis/Audio/Music/MSS_AtlantisAmbientMusic.MSS_AtlantisAmbientMusic");
	const TCHAR* ScoreBPath = TEXT("/Game/ProjectAtlantis/Audio/MetaSoundLibrary/MSS_Stereo_Looping_lib.MSS_Stereo_Looping_lib");
}

BEGIN_DEFINE_SPEC(FAtlantisMusicPlaybackSpec, "Atlantis.Audio.MusicPlayback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UWorld> World;

	/** The project's Music Per Map table, put back after the tests that replace it. */
	TMap<TSoftObjectPtr<UWorld>, TSoftObjectPtr<UMetaSoundSource>> SavedMusicPerMap;

	/** A stand-in world for a map package. It isn't loaded or initialized; only its package name and type matter. */
	void MakeWorld(const TCHAR* PackageName, EWorldType::Type WorldType)
	{
		World.Reset(NewObject<UWorld>(CreatePackage(PackageName), NAME_None, RF_Transient));
		World->WorldType = WorldType;
	}

	/** Adds a test row to Music Per Map: the map package name, and the score's object path (empty = no score set). */
	static void AddMusicRow(const TCHAR* MapPackageName, const TCHAR* ScorePath)
	{
		const FString MapPath = FString::Printf(TEXT("%s.%s"), MapPackageName, *FPackageName::GetShortName(MapPackageName));
		FAtlantisAudioValidationTestAccess::MusicPerMap().Add(
			TSoftObjectPtr<UWorld>(FSoftObjectPath(MapPath)),
			ScorePath ? TSoftObjectPtr<UMetaSoundSource>(FSoftObjectPath(ScorePath)) : TSoftObjectPtr<UMetaSoundSource>());
	}
END_DEFINE_SPEC(FAtlantisMusicPlaybackSpec)

void FAtlantisMusicPlaybackSpec::Define()
{
	AfterEach([this]()
	{
		World.Reset();
	});

	// These check the project's own Music Per Map table (Project Settings > Atlantis Audio).
	Describe("Gameplay map selection", [this]()
	{
		It("should enable music in the tutorial, district entrance and District 1", [this]()
		{
			for (const TCHAR* Map : {
				TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_Main"),
				TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_District_Entrance"),
				TEXT("/Game/ProjectAtlantis/Maps/L_District1_Cityhub")})
			{
				MakeWorld(Map, EWorldType::Game);
				TestTrue(Map, UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			}
		});

		It("should recognize a PIE copy of the tutorial map", [this]()
		{
			MakeWorld(TEXT("/Game/ProjectAtlantis/Maps/Tutorial/UEDPIE_2_L_Tutorial_Main"), EWorldType::PIE);
			TestTrue(TEXT("PIE tutorial has music"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
		});

		It("should leave main menu music to its existing owner", [this]()
		{
			MakeWorld(TEXT("/Game/ProjectAtlantis/Maps/L_MainMenu"), EWorldType::Game);
			TestFalse(TEXT("No subsystem music in menu"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
		});

		It("should not start music in editor worlds or unconfigured maps", [this]()
		{
			MakeWorld(TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_Main"), EWorldType::Editor);
			TestFalse(TEXT("No editor playback"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			MakeWorld(TEXT("/Game/Tests/UnconfiguredMusicMap"), EWorldType::Game);
			TestFalse(TEXT("No unconfigured map playback"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			TestFalse(TEXT("No null world playback"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(nullptr));
		});
	});

	// These add their own test maps to Music Per Map, so they don't depend on the project's table.
	Describe("Score per map", [this]()
	{
		BeforeEach([this]()
		{
			SavedMusicPerMap = FAtlantisAudioValidationTestAccess::MusicPerMap();
			AddMusicRow(TEXT("/Game/Tests/MusicMapA"), ScoreAPath);
			AddMusicRow(TEXT("/Game/Tests/MusicMapB"), ScoreBPath);
			AddMusicRow(TEXT("/Game/Tests/MusicMapNoScore"), nullptr);
		});

		AfterEach([this]()
		{
			FAtlantisAudioValidationTestAccess::MusicPerMap() = SavedMusicPerMap;
		});

		It("should give each map its own score", [this]()
		{
			MakeWorld(TEXT("/Game/Tests/MusicMapA"), EWorldType::Game);
			const UMetaSoundSource* ScoreA = UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get());
			MakeWorld(TEXT("/Game/Tests/MusicMapB"), EWorldType::Game);
			const UMetaSoundSource* ScoreB = UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get());

			if (TestNotNull(TEXT("Map A has a score"), ScoreA) && TestNotNull(TEXT("Map B has a score"), ScoreB))
			{
				TestEqual(TEXT("Map A plays score A"), ScoreA->GetPathName(), FString(ScoreAPath));
				TestEqual(TEXT("Map B plays score B"), ScoreB->GetPathName(), FString(ScoreBPath));
			}
		});

		It("should give a PIE copy of a map that map's score", [this]()
		{
			MakeWorld(TEXT("/Game/Tests/UEDPIE_0_MusicMapB"), EWorldType::PIE);
			const UMetaSoundSource* Score = UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get());
			if (TestNotNull(TEXT("PIE copy has a score"), Score))
			{
				TestEqual(TEXT("PIE copy plays map B's score"), Score->GetPathName(), FString(ScoreBPath));
			}
		});

		It("should give no score to unlisted maps, editor worlds or a null world", [this]()
		{
			MakeWorld(TEXT("/Game/Tests/UnconfiguredMusicMap"), EWorldType::Game);
			TestNull(TEXT("Unlisted map"), UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get()));
			MakeWorld(TEXT("/Game/Tests/MusicMapA"), EWorldType::Editor);
			TestNull(TEXT("Editor world"), UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get()));
			TestNull(TEXT("Null world"), UAtlantisAudioDeveloperSettings::GetMusicForWorld(nullptr));
		});

		It("should log an error and give no score when a listed map has no score set", [this]()
		{
			AddExpectedErrorPlain(TEXT("is listed but no score is set"), EAutomationExpectedErrorFlags::Contains, 1);

			MakeWorld(TEXT("/Game/Tests/MusicMapNoScore"), EWorldType::Game);
			TestTrue(TEXT("Listed map counts as a music map"), UAtlantisAudioDeveloperSettings::IsMusicPlaybackWorld(World.Get()));
			TestNull(TEXT("No score to play"), UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get()));
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

		It("should load a stereo music source with music mixing routes for the tutorial", [this]()
		{
			MakeWorld(TEXT("/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_Main"), EWorldType::Game);
			UMetaSoundSource* Music = UAtlantisAudioDeveloperSettings::GetMusicForWorld(World.Get());
			if (TestNotNull(TEXT("Configured tutorial music loads"), Music))
			{
				TestEqual(TEXT("Stereo output"), Music->NumChannels, 2);
				TestNotNull(TEXT("Music sound class"), Music->SoundClassObject.Get());
				TestNotNull(TEXT("Music submix"), Music->SoundSubmixObject.Get());
			}
		});
	});
}

#endif
