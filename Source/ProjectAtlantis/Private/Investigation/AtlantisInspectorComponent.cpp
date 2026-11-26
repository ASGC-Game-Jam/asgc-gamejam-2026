// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectorComponent.h"
#include "Investigation/AtlantisInspectableComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspector, Log, All);

UAtlantisInspectorComponent::UAtlantisInspectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UAtlantisInspectorComponent::BeginInspect(UAtlantisInspectableComponent* Inspectable)
{
	if (!Inspectable || IsInspecting())
	{
		return false;
	}

	InspectedObject = Inspectable;
	UE_LOG(LogAtlantisInspector, Log, TEXT("Inspecting %s (\"%s\")"),
		*GetNameSafe(Inspectable->GetOwner()), *Inspectable->DisplayName.ToString());
	return true;
}

void UAtlantisInspectorComponent::EndInspect()
{
	if (!IsInspecting())
	{
		return;
	}

	UE_LOG(LogAtlantisInspector, Log, TEXT("Stopped inspecting %s"), *GetNameSafe(InspectedObject->GetOwner()));
	InspectedObject.Reset();
}
