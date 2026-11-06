// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Debug/AtlantisDebugMenuSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
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
		TStrongObjectPtr<UInputMappingContext> Gameplay;
		TStrongObjectPtr<UInputMappingContext> Menu;
		FAtlantisDebugInputMappings Mappings;
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

		void BuildMappings()
		{
			Mappings = Settings->CreateInputMappings(Settings.Get());
			Gameplay.Reset(Mappings.Gameplay);
			Menu.Reset(Mappings.Menu);
		}

		static bool HasKey(const UInputMappingContext* Context, const FKey& Key)
		{
			return Context && Context->GetMappings().ContainsByPredicate(
				[&Key](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Key == Key; });
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

	Describe(TEXT("Enhanced Input mappings"), [this]()
	{
		It(TEXT("should apply configured keys and leave F9 for screenshot capture"), [this]()
		{
			Fixture->BuildMappings();
			for (const UInputMappingContext* Context : {Fixture->Mappings.Gameplay, Fixture->Mappings.Menu})
			{
				TestTrue(TEXT("Configured toggle key"), Fixture->HasKey(Context, EKeys::F6));
				TestTrue(TEXT("Configured reset key"), Fixture->HasKey(Context, EKeys::F8));
				TestFalse(TEXT("Old toggle key released"), Fixture->HasKey(Context, EKeys::F7));
				TestFalse(TEXT("Old reset key released"), Fixture->HasKey(Context, EKeys::F10));
				TestFalse(TEXT("F9 remains unmapped"), Fixture->HasKey(Context, EKeys::F9));
			}
		});

		It(TEXT("should omit configurable shortcuts set to None"), [this]()
		{
			Fixture->BuildMappings();
			for (const UInputMappingContext* Context : {Fixture->Mappings.Gameplay, Fixture->Mappings.Menu})
			{
				TestFalse(TEXT("Gamepad toggle disabled"), Fixture->HasKey(Context, EKeys::Gamepad_Special_Right));
				TestFalse(TEXT("Gamepad reset disabled"), Fixture->HasKey(Context, EKeys::Gamepad_RightThumbstick));
				TestFalse(TEXT("Invalid key never mapped"), Fixture->HasKey(Context, FKey()));
			}
		});

		It(TEXT("should use an engine chord to open and reserve Back and stick reset for the menu"), [this]()
		{
			Fixture->Settings->ToggleMenuGamepadKey = EKeys::Gamepad_Special_Right;
			Fixture->Settings->ToggleMenuGamepadModifier = EKeys::Gamepad_LeftThumbstick;
			Fixture->Settings->ResetPlayerGamepadKey = EKeys::Gamepad_RightThumbstick;
			Fixture->BuildMappings();
			const FEnhancedActionKeyMapping* ToggleMapping = Fixture->Gameplay->GetMappings().FindByPredicate(
				[](const FEnhancedActionKeyMapping& Mapping) { return Mapping.Key == EKeys::Gamepad_Special_Right; });
			if (!TestNotNull(TEXT("Gamepad toggle mapping"), ToggleMapping))
			{
				return;
			}
			const UInputTriggerChordAction* Chord = ToggleMapping->Triggers.Num() == 1
				? Cast<UInputTriggerChordAction>(ToggleMapping->Triggers[0]) : nullptr;
			if (!TestNotNull(TEXT("Native chord trigger"), Chord))
			{
				return;
			}
			TestTrue(TEXT("Modifier uses the left stick"), Fixture->Gameplay->GetMappings().ContainsByPredicate(
				[Chord](const FEnhancedActionKeyMapping& Mapping)
				{
					return Mapping.Action == Chord->ChordAction && Mapping.Key == EKeys::Gamepad_LeftThumbstick;
				}));
			TestFalse(TEXT("Modifier does not consume gameplay input"), Chord->ChordAction->bConsumeInput);
			TestTrue(TEXT("Menu keeps the same toggle action without a chord"), Fixture->Menu->GetMappings().ContainsByPredicate(
				[this](const FEnhancedActionKeyMapping& Mapping)
				{
					return Mapping.Key == EKeys::Gamepad_Special_Right
						&& Mapping.Action == Fixture->Mappings.GamepadToggle && Mapping.Triggers.IsEmpty();
				}));
			TestFalse(TEXT("Gameplay B is untouched"), Fixture->HasKey(Fixture->Gameplay.Get(), EKeys::Gamepad_FaceButton_Right));
			TestFalse(TEXT("Gameplay right stick is untouched"), Fixture->HasKey(Fixture->Gameplay.Get(), EKeys::Gamepad_RightThumbstick));
			TestTrue(TEXT("Menu Back is available"), Fixture->HasKey(Fixture->Menu.Get(), EKeys::Gamepad_FaceButton_Right));
			TestTrue(TEXT("Menu reset is available"), Fixture->HasKey(Fixture->Menu.Get(), EKeys::Gamepad_RightThumbstick));
		});
	});
}

#endif
