// Copyright (c) 2026 ASGC

#pragma once

#include "Misc/Parse.h"

namespace ProjectAtlantis::ScreenshotCapture
{
	inline bool IsEnabled(bool bShippingBuild, const TCHAR* CommandLine)
	{
		return !bShippingBuild || FParse::Param(CommandLine, TEXT("debug"));
	}
}
