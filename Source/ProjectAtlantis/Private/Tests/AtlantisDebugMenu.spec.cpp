// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Debug/AtlantisDebugMenuSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisDebugMenuAvailabilityTest, "Atlantis.Debug.Availability",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisDebugMenuAvailabilityTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Non-Shipping enables debug features by default"), UAtlantisDebugMenuSettings::IsAvailable(false, TEXT("")));
	TestFalse(TEXT("Shipping requires an opt-in"), UAtlantisDebugMenuSettings::IsAvailable(true, TEXT("")));
	TestFalse(TEXT("A similar option does not opt in"), UAtlantisDebugMenuSettings::IsAvailable(true, TEXT("-debugger")));
	TestTrue(TEXT("Shipping accepts -debug alongside other launch options"),
		UAtlantisDebugMenuSettings::IsAvailable(true, TEXT("-windowed -debug -ResX=1280")));
	return true;
}

#endif
