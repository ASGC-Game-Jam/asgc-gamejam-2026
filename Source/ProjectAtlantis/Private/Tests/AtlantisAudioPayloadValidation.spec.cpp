// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Misc/AutomationTest.h"
#include "NativeGameplayTags.h"
#include "StructUtils/InstancedStruct.h"

namespace
{
	// Test-only tags. Native tags register themselves at startup, so they need no .ini entry.
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioValidation_Vector, "Test.Atlantis.AudioValidation.Vector");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioValidation_Blank, "Test.Atlantis.AudioValidation.Blank");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Test_AudioValidation_Unregistered, "Test.Atlantis.AudioValidation.Unregistered");

	using FPayloadRegistry = TMap<FGameplayTag, TSoftObjectPtr<const UScriptStruct>>;
}

// Declared as a friend in AtlantisAudioDeveloperSettings.h so tests can reach the protected registry.
struct FAtlantisAudioValidationTestAccess
{
	static FPayloadRegistry& Registry()
	{
		return GetMutableDefault<UAtlantisAudioDeveloperSettings>()->PayloadRegistry;
	}
};

BEGIN_DEFINE_SPEC(FAtlantisAudioPayloadValidationSpec, "Atlantis.Audio.PayloadValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FPayloadRegistry SavedRegistry;

	// ensure only fires once per editor session, so its log line is ignored rather than counted.
	void ExpectBreach(const TCHAR* MessageFragment)
	{
		AddExpectedErrorPlain(TEXT("Ensure condition failed"), EAutomationExpectedErrorFlags::Contains, -1);
		AddExpectedErrorPlain(MessageFragment, EAutomationExpectedErrorFlags::Contains, 0);
	}
END_DEFINE_SPEC(FAtlantisAudioPayloadValidationSpec)

void FAtlantisAudioPayloadValidationSpec::Define()
{
	BeforeEach([this]()
	{
		// The registry is the live Project Settings object, so stash the real rows and restore them afterwards.
		FPayloadRegistry& Registry = FAtlantisAudioValidationTestAccess::Registry();
		SavedRegistry = Registry;
		Registry.Reset();
		Registry.Add(TAG_Test_AudioValidation_Vector, TBaseStructure<FVector>::Get());
		Registry.Add(TAG_Test_AudioValidation_Blank, TSoftObjectPtr<const UScriptStruct>());
	});

	AfterEach([this]()
	{
		FAtlantisAudioValidationTestAccess::Registry() = SavedRegistry;
	});

	It(TEXT("should accept a payload that matches its registered struct"), [this]()
	{
		TestTrue(TEXT("Matching payload is valid"),
			UAtlantisAudioDeveloperSettings::ValidatePayload(TAG_Test_AudioValidation_Vector, FInstancedStruct::Make(FVector::ZeroVector)));
	});

	Describe(TEXT("Contract breaches"), [this]()
	{
		It(TEXT("should reject an empty tag"), [this]()
		{
			ExpectBreach(TEXT("empty/invalid GameplayTag"));
			TestFalse(TEXT("Empty tag is rejected"),
				UAtlantisAudioDeveloperSettings::ValidatePayload(FGameplayTag(), FInstancedStruct::Make(FVector::ZeroVector)));
		});

		It(TEXT("should reject a tag that is not registered"), [this]()
		{
			ExpectBreach(TEXT("is not registered"));
			TestFalse(TEXT("Unregistered tag is rejected"),
				UAtlantisAudioDeveloperSettings::ValidatePayload(TAG_Test_AudioValidation_Unregistered, FInstancedStruct::Make(FVector::ZeroVector)));
		});

		It(TEXT("should reject a registered tag with no struct selected"), [this]()
		{
			ExpectBreach(TEXT("struct is empty or missing"));
			TestFalse(TEXT("Blank registry row is rejected"),
				UAtlantisAudioDeveloperSettings::ValidatePayload(TAG_Test_AudioValidation_Blank, FInstancedStruct::Make(FVector::ZeroVector)));
		});

		It(TEXT("should reject a payload holding the wrong struct"), [this]()
		{
			ExpectBreach(TEXT("but the registry expects"));
			TestFalse(TEXT("Mismatched payload is rejected"),
				UAtlantisAudioDeveloperSettings::ValidatePayload(TAG_Test_AudioValidation_Vector, FInstancedStruct::Make(FRotator::ZeroRotator)));
		});

		It(TEXT("should reject an empty payload"), [this]()
		{
			ExpectBreach(TEXT("but the registry expects"));
			TestFalse(TEXT("Empty payload is rejected"),
				UAtlantisAudioDeveloperSettings::ValidatePayload(TAG_Test_AudioValidation_Vector, FInstancedStruct()));
		});
	});
}

#endif
