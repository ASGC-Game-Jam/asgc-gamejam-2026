// Copyright (c) 2026 ASGC

#include "UI/AtlantisDetectionMarkerWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UAtlantisDetectionMarkerWidget::SetDetection(float InMeter, EMonsterStatus InStatus)
{
	Meter = FMath::Clamp(InMeter, 0.f, 1.f);
	Status = InStatus;
	Refresh();
}

float UAtlantisDetectionMarkerWidget::GetDisplayFill() const
{
	return Status == EMonsterStatus::Combat ? 1.f : Meter;
}

FLinearColor UAtlantisDetectionMarkerWidget::GetStatusColor() const
{
	switch (Status)
	{
	case EMonsterStatus::Alerted:   return AlertedColor;
	case EMonsterStatus::Searching: return SearchingColor;
	case EMonsterStatus::Combat:    return CombatColor;
	default:                        return IdleColor;
	}
}

void UAtlantisDetectionMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UAtlantisDetectionMarkerWidget::Refresh()
{
	if (FillBar != nullptr)
	{
		FillBar->SetPercent(GetDisplayFill());
		FillBar->SetFillColorAndOpacity(GetStatusColor());
	}

	if (StatusText != nullptr)
	{
		const TCHAR* Icon = TEXT("");
		if (Status == EMonsterStatus::Combat)
		{
			Icon = TEXT("!");
		}
		else if (Status == EMonsterStatus::Alerted || Status == EMonsterStatus::Searching)
		{
			Icon = TEXT("?");
		}
		StatusText->SetText(FText::FromString(Icon));
		StatusText->SetColorAndOpacity(FSlateColor(GetStatusColor()));
	}

	OnDetectionUpdated(Meter, Status);
}
