// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "AtlantisScreenshotCaptureSettings.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	// Keep each spec's settings and config file separate from the project's live configuration.
	struct FAtlantisScreenshotCaptureBindingFixture
	{
		TStrongObjectPtr<UAtlantisScreenshotCaptureSettings> Settings;
		FString TestConfig;
		bool bConfigWritten = false;

		FAtlantisScreenshotCaptureBindingFixture()
			: Settings(NewObject<UAtlantisScreenshotCaptureSettings>())
		{
			const FString TestDirectory = FPaths::ProjectSavedDir() / TEXT("Automation");
			IFileManager::Get().MakeDirectory(*TestDirectory, true);
			TestConfig = FPaths::ConvertRelativePathToFull(TestDirectory
				/ FString::Printf(TEXT("ScreenshotBinding_%s.ini"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
			bConfigWritten = FFileHelper::SaveStringToFile(
				TEXT("[/Script/ScreenshotCapture.AtlantisScreenshotCaptureSettings]\nCaptureKey=F8\n"), *TestConfig);
			if (bConfigWritten)
			{
				Settings->LoadConfig(nullptr, *TestConfig);
			}
		}

		~FAtlantisScreenshotCaptureBindingFixture()
		{
			GConfig->UnloadFile(TestConfig);
			IFileManager::Get().Delete(*TestConfig);
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisScreenshotCaptureBindingSpec, "Atlantis.ScreenshotCapture.ConfiguredBinding",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisScreenshotCaptureBindingFixture> Fixture;
END_DEFINE_SPEC(FAtlantisScreenshotCaptureBindingSpec)

void FAtlantisScreenshotCaptureBindingSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisScreenshotCaptureBindingFixture>();
		TestNotNull(TEXT("Capture settings created"), Fixture->Settings.Get());
		TestTrue(TEXT("Custom capture binding written"), Fixture->bConfigWritten);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("GameConfig"), [this]()
	{
		It(TEXT("should replace the default shortcut with the configured key"), [this]()
		{
			if (!Fixture->Settings.IsValid() || !Fixture->bConfigWritten)
			{
				return;
			}
			TestEqual(TEXT("Game config replaces the default capture key"), Fixture->Settings->CaptureKey, EKeys::F8);
			TestNotEqual(TEXT("The original shortcut is no longer bound"), Fixture->Settings->CaptureKey, EKeys::F9);
		});
	});
}

#endif
