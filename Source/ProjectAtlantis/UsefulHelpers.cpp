// Copyright (c) 2026 ASGC


#include "UsefulHelpers.h"

void UUsefulHelpers::GetProjectVersion(FString& version)
{
	version = GetDefault<UGeneralProjectSettings>()->ProjectVersion;
}
