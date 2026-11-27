// Copyright (c) 2026 ASGC

#include "UI/AtlantisInspectWidget.h"
#include "Investigation/AtlantisInspectorComponent.h"

void UAtlantisInspectWidget::StartInspect(UAtlantisInspectorComponent* InInspector, UAtlantisInspectableComponent* Inspectable)
{
	Inspector = InInspector;
	OnInspectStarted(Inspectable);
}

void UAtlantisInspectWidget::EndInspect(UAtlantisInspectableComponent* Inspectable)
{
	OnInspectEnded(Inspectable);
	Inspector.Reset();
}
