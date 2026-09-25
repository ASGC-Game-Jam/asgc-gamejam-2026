// Copyright (c) 2026 ASGC


#include "Core/AtlantisEnemyController.h"

#include "Perception/AIPerceptionComponent.h"


// Sets default values
AAtlantisEnemyController::AAtlantisEnemyController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>("AI Perception");
}