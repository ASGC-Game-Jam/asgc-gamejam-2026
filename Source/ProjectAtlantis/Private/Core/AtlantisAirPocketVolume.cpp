// Copyright (c) 2026 ASGC
#include "Core/AtlantisAirPocketVolume.h"
#include "Core/AtlantisOxygenComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Components/BrushComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"

AAtlantisAirPocketVolume::AAtlantisAirPocketVolume()
{
	PrimaryActorTick.bCanEverTick = true;
	Priority = 100;
	bWaterVolume = false;
	bPhysicsOnContact = false;
	GetBrushComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AirBoundary = CreateDefaultSubobject<USphereComponent>(TEXT("AirBoundary"));
	AirBoundary->SetMobility(EComponentMobility::Static);
	SetRootComponent(AirBoundary);
	GetBrushComponent()->SetupAttachment(AirBoundary);
	AirBoundary->InitSphereRadius(700.f);
	AirBoundary->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	AirBoundary->SetGenerateOverlapEvents(true);
}

void AAtlantisAirPocketVolume::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || OxygenRestoreRate <= 0.f)
	{
		return;
	}
	TArray<AActor*> Occupants;
	AirBoundary->GetOverlappingActors(Occupants, ACharacter::StaticClass());
	for (AActor* Occupant : Occupants)
	{
		ACharacter* Character = Cast<ACharacter>(Occupant);
		if (!Character || Character->GetPhysicsVolume() != this)
		{
			continue;
		}
		AAtlantisPlayerState* State = Character->GetPlayerState<AAtlantisPlayerState>();
		if (State && State->GetOxygenComponent())
		{
			State->GetOxygenComponent()->RestoreOxygen(OxygenRestoreRate * DeltaSeconds, false);
		}
	}
}
