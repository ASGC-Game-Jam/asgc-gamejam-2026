// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Debug/AtlantisDebugMenuSettings.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FAtlantisDebugMenuBindingFixture
	{
		TStrongObjectPtr<UAtlantisDebugMenuSettings> Settings;
		FString TestConfig;
		bool bConfigWritten = false;

		FAtlantisDebugMenuBindingFixture()
			: Settings(NewObject<UAtlantisDebugMenuSettings>())
		{
			const FString TestDirectory = FPaths::ProjectSavedDir() / TEXT("Automation");
			IFileManager::Get().MakeDirectory(*TestDirectory, true);
			TestConfig = FPaths::ConvertRelativePathToFull(TestDirectory
				/ FString::Printf(TEXT("DebugMenuBinding_%s.ini"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
			bConfigWritten = FFileHelper::SaveStringToFile(
				TEXT("[/Script/ProjectAtlantis.AtlantisDebugMenuSettings]\n")
				TEXT("ToggleMenuKey=F6\nToggleMenuGamepadKey=None\n")
				TEXT("ResetPlayerKey=F8\nResetPlayerGamepadKey=None\n"), *TestConfig);
			if (bConfigWritten)
			{
				Settings->LoadConfig(nullptr, *TestConfig);
			}
		}

		~FAtlantisDebugMenuBindingFixture()
		{
			GConfig->UnloadFile(TestConfig);
			IFileManager::Get().Delete(*TestConfig);
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisDebugMenuBindingSpec, "Atlantis.Debug.ConfiguredBindings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisDebugMenuBindingFixture> Fixture;
END_DEFINE_SPEC(FAtlantisDebugMenuBindingSpec)

void FAtlantisDebugMenuBindingSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisDebugMenuBindingFixture>();
		TestTrue(TEXT("Isolated binding config written"), Fixture->bConfigWritten);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("Game config"), [this]()
	{
		It(TEXT("should replace menu and reset shortcuts while leaving F9 for screenshot capture"), [this]()
		{
			if (!Fixture->bConfigWritten)
			{
				return;
			}
			TestTrue(TEXT("Configured menu key"), Fixture->Settings->IsToggleKey(EKeys::F6));
			TestFalse(TEXT("Original menu key released"), Fixture->Settings->IsToggleKey(EKeys::F7));
			TestTrue(TEXT("Configured reset key"), Fixture->Settings->IsResetKey(EKeys::F8));
			TestFalse(TEXT("Original reset key released"), Fixture->Settings->IsResetKey(EKeys::F10));
			TestFalse(TEXT("F9 does not open menu"), Fixture->Settings->IsToggleKey(EKeys::F9));
			TestFalse(TEXT("F9 does not reset player"), Fixture->Settings->IsResetKey(EKeys::F9));
		});

		It(TEXT("should disable gamepad shortcuts configured as None"), [this]()
		{
			if (!Fixture->bConfigWritten)
			{
				return;
			}
			TestFalse(TEXT("Gamepad toggle disabled"), Fixture->Settings->IsToggleKey(EKeys::Gamepad_Special_Left));
			TestFalse(TEXT("Gamepad reset disabled"), Fixture->Settings->IsResetKey(EKeys::Gamepad_RightThumbstick));
			TestFalse(TEXT("Invalid key cannot toggle"), Fixture->Settings->IsToggleKey(FKey()));
			TestFalse(TEXT("Invalid key cannot reset"), Fixture->Settings->IsResetKey(FKey()));
		});
	});
}

#endif
