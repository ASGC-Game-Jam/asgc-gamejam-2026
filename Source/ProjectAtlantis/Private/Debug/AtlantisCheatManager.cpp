// Copyright (c) 2026 ASGC

#include "Debug/AtlantisCheatManager.h"
#include "Debug/AtlantisDebugMenuSettings.h"
#include "Core/AtlantisPlayerController.h"

UAtlantisCheatManager::UAtlantisCheatManager()
{
	FAtlantisDebugAction Action;
	Action.Label = NSLOCTEXT("AtlantisDebug", "RestartGame", "Restart Game");
	Action.Command = TEXT("RestartGame");
	DebugActions.Add(Action);
}

void UAtlantisCheatManager::RestartGame()
{
	if (AAtlantisPlayerController* Controller = Cast<AAtlantisPlayerController>(GetOuter()))
	{
		Controller->RequestDebugRestart();
	}
}

TArray<FAtlantisDebugAction> UAtlantisCheatManager::GetDebugActions_Implementation() const
{
	if (!UAtlantisDebugMenuSettings::IsAvailable())
	{
		return {};
	}
	return DebugActions;
}
