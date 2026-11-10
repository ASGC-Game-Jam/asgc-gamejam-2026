// Copyright (c) 2026 ASGC


#include "Core/AtlantisEnemyController.h"

#include "AI/AtlantisSightConeComponent.h"
#include "Perception/AIPerceptionComponent.h"


// Sets default values
AAtlantisEnemyController::AAtlantisEnemyController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>("AI Perception");
	SightCone = CreateDefaultSubobject<UAtlantisSightConeComponent>("Sight Cone");
}

void AAtlantisEnemyController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

#if ENABLE_DRAW_DEBUG
	if (SightCone != nullptr)
	{
		SightCone->DrawSightCone(AIPerception, GetPawn(), CurrentStatus);
	}
#endif // ENABLE_DRAW_DEBUG
}

void AAtlantisEnemyController::SetStatus(EMonsterStatus InStatus)
{
	if (CurrentStatus == InStatus)
	{
		return;
	}

	const EMonsterStatus OldStatus = CurrentStatus;
	CurrentStatus = InStatus;
	OnStatusChanged.Broadcast(OldStatus, CurrentStatus);
}
