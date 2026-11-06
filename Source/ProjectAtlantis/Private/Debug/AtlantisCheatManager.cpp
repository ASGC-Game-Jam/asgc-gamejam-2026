// Copyright (c) 2026 ASGC

#include "Debug/AtlantisCheatManager.h"
#include "Core/AtlantisPlayerController.h"

UAtlantisCheatManager::UAtlantisCheatManager()
{
#if !UE_BUILD_SHIPPING
	FAtlantisDebugAction Action;
	Action.Label = NSLOCTEXT("AtlantisDebug", "RestartGame", "Restart Game");
	Action.Command = TEXT("RestartGame");
	DebugActions.Add(Action);
#endif
}

void UAtlantisCheatManager::RestartGame()
{
#if !UE_BUILD_SHIPPING
	if (AAtlantisPlayerController* Controller = Cast<AAtlantisPlayerController>(GetOuter()))
	{
		Controller->RequestDebugRestart();
	}
#endif
}

TArray<FAtlantisDebugAction> UAtlantisCheatManager::GetDebugActions_Implementation() const
{
#if !UE_BUILD_SHIPPING
	return DebugActions;
#else
	return {};
#endif
}
