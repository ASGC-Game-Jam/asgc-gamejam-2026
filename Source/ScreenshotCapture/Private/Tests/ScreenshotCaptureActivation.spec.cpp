// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "ScreenshotCaptureSettings.h"
#include "Misc/AutomationTest.h"

BEGIN_DEFINE_SPEC(FAtlantisScreenshotCaptureActivationSpec, "Atlantis.ScreenshotCapture.Activation",
                  EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FAtlantisScreenshotCaptureActivationSpec)

void FAtlantisScreenshotCaptureActivationSpec::Define()
{
	Describe(TEXT("NonShipping"), [this]()
	{
		It(TEXT("should enable capture without launch options"), [this]()
		{
			TestTrue(TEXT("Non-Shipping capture is enabled without launch options"),
			         ProjectAtlantis::ScreenshotCapture::IsEnabled(false, TEXT("")));
		});
	});

	Describe(TEXT("Shipping"), [this]()
	{
		It(TEXT("should leave capture inactive without launch options"), [this]()
		{
			TestFalse(TEXT("Shipping capture is inactive without launch options"),
			          ProjectAtlantis::ScreenshotCapture::IsEnabled(true, TEXT("")));
		});

		It(TEXT("should enable capture with the debug launch option"), [this]()
		{
			TestTrue(TEXT("Steam's debug launch option enables Shipping capture"),
			         ProjectAtlantis::ScreenshotCapture::IsEnabled(true, TEXT("-debug")));
		});

		It(TEXT("should accept debug alongside other launch options"), [this]()
		{
			TestTrue(TEXT("Debug can appear alongside other launch options"),
			         ProjectAtlantis::ScreenshotCapture::IsEnabled(true, TEXT("-windowed -debug -ResX=1280")));
		});

		It(TEXT("should match the debug option case insensitively"), [this]()
		{
			TestTrue(TEXT("Debug option is case insensitive"),
			         ProjectAtlantis::ScreenshotCapture::IsEnabled(true, TEXT("-DEBUG")));
		});

		It(TEXT("should reject other debug-prefixed options"), [this]()
		{
			TestFalse(TEXT("Other debug-prefixed options do not enable capture"),
			          ProjectAtlantis::ScreenshotCapture::IsEnabled(true, TEXT("-debugger")));
		});

		It(TEXT("should reject a negative debug option"), [this]()
		{
			TestFalse(TEXT("A negative debug option does not enable capture"),
			          ProjectAtlantis::ScreenshotCapture::IsEnabled(true, TEXT("-nodebug")));
		});
	});
}

#endif
