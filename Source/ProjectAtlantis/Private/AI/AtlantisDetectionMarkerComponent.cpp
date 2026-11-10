// Copyright (c) 2026 ASGC

#include "AI/AtlantisDetectionMarkerComponent.h"

#include "UI/AtlantisDetectionMarkerWidget.h"

UAtlantisDetectionMarkerComponent::UAtlantisDetectionMarkerComponent()
{
	SetWidgetSpace(EWidgetSpace::Screen);
	SetDrawAtDesiredSize(true);
	SetPivot(FVector2D(0.5f, 1.f));
	// Above a default-sized capsule (half height 90).
	SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	CanCharacterStepUpOn = ECB_No;
	SetHiddenInGame(true);
}

void UAtlantisDetectionMarkerComponent::SetDetection(float InMeter, EMonsterStatus InStatus)
{
	Meter = FMath::Clamp(InMeter, 0.f, 1.f);
	Status = InStatus;
	PushToWidget();
}

void UAtlantisDetectionMarkerComponent::InitWidget()
{
	Super::InitWidget();
	PushToWidget();
}

void UAtlantisDetectionMarkerComponent::PushToWidget()
{
	const bool bCalm = Status == EMonsterStatus::Idle && Meter <= KINDA_SMALL_NUMBER;
	SetHiddenInGame(bHideWhenCalm && bCalm);

	UUserWidget* const UserWidget = GetUserWidgetObject();
	if (UserWidget == nullptr)
	{
		return;
	}

	if (UAtlantisDetectionMarkerWidget* const Marker = Cast<UAtlantisDetectionMarkerWidget>(UserWidget))
	{
		Marker->SetDetection(Meter, Status);
	}
	else if (!bHasLoggedWrongWidgetClass)
	{
		bHasLoggedWrongWidgetClass = true;
		UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] %s on %s: Widget Class %s is not a child of AtlantisDetectionMarkerWidget; the marker won't update."),
			__FUNCTION__, *GetName(), *GetNameSafe(GetOwner()), *GetNameSafe(UserWidget->GetClass()));
	}
}
