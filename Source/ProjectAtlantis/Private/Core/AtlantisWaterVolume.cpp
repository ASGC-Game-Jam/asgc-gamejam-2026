// Copyright (c) 2026 ASGC
#include "Core/AtlantisWaterVolume.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"

AAtlantisWaterVolume::AAtlantisWaterVolume()
{
	Priority = 10;
	bWaterVolume = true;
	bPhysicsOnContact = false;
	FluidFriction = 0.3f;
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterBoundary = CreateDefaultSubobject<UBoxComponent>(TEXT("WaterBoundary"));
	WaterBoundary->SetMobility(EComponentMobility::Static);
	SetRootComponent(WaterBoundary);
	GetBrushComponent()->SetupAttachment(WaterBoundary);
	WaterBoundary->InitBoxExtent(FVector(10000.f, 10000.f, 2500.f));
	WaterBoundary->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	WaterBoundary->SetGenerateOverlapEvents(true);
}
