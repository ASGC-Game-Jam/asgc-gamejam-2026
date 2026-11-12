// Copyright (c) 2026 ASGC

#include "AI/Navigation/AtlantisNavGraphLibrary.h"

#include "AI/Navigation/AtlantisNavGraphSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

FString UAtlantisNavGraphLibrary::BuildAndDescribeNavGraph(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine != nullptr ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	UAtlantisNavGraphSubsystem* NavGraph = World != nullptr ? World->GetSubsystem<UAtlantisNavGraphSubsystem>() : nullptr;
	if (NavGraph == nullptr)
	{
		return TEXT("No nav graph subsystem in this world.");
	}
	return NavGraph->Describe();
}
