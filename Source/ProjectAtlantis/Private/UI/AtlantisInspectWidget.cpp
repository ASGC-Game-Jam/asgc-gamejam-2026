// Copyright (c) 2026 ASGC

#include "UI/AtlantisInspectWidget.h"
#include "Investigation/AtlantisInspectorComponent.h"
#include "Investigation/AtlantisInspectPartFocusComponent.h"
#include "GameFramework/Actor.h"

void UAtlantisInspectWidget::StartInspect(UAtlantisInspectorComponent* InInspector, UAtlantisInspectableComponent* Inspectable)
{
	Inspector = InInspector;
	OnInspectStarted(Inspectable);

	UAtlantisInspectPartFocusComponent* Focus = InInspector && InInspector->GetOwner()
		? InInspector->GetOwner()->FindComponentByClass<UAtlantisInspectPartFocusComponent>() : nullptr;
	if (Focus)
	{
		Focus->OnPartFocusChanged.AddDynamic(this, &UAtlantisInspectWidget::HandlePartFocusChanged);
		PartFocus = Focus;

		// The part focus may have reacted to the inspect starting before we were created
		if (UAtlantisInspectPartComponent* Focused = Focus->GetFocusedPart())
		{
			OnPartFocusChanged(Focused);
		}
	}
}

void UAtlantisInspectWidget::EndInspect(UAtlantisInspectableComponent* Inspectable)
{
	if (UAtlantisInspectPartFocusComponent* Focus = PartFocus.Get())
	{
		Focus->OnPartFocusChanged.RemoveDynamic(this, &UAtlantisInspectWidget::HandlePartFocusChanged);
	}
	PartFocus.Reset();

	OnInspectEnded(Inspectable);
	Inspector.Reset();
}

bool UAtlantisInspectWidget::GetFocusedPartScreenPosition(FVector2D& ScreenPosition) const
{
	const UAtlantisInspectPartFocusComponent* Focus = PartFocus.Get();
	return Focus && Focus->GetFocusedPartScreenPosition(ScreenPosition);
}

void UAtlantisInspectWidget::HandlePartFocusChanged(UAtlantisInspectPartComponent* NewPart, UAtlantisInspectPartComponent* OldPart)
{
	OnPartFocusChanged(NewPart);
}
