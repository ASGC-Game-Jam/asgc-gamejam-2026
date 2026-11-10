// Copyright (c) 2026 ASGC


#include "Core/AtlantisEnemyController.h"

#include "AI/AtlantisSightConeComponent.h"
#include "AITypes.h"
#include "Perception/AIPerceptionComponent.h"


// Sets default values
AAtlantisEnemyController::AAtlantisEnemyController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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

void AAtlantisEnemyController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	// Not calling Super on purpose: the engine version snaps straight to the focal point.
	APawn* const MyPawn = GetPawn();
	if (MyPawn == nullptr)
	{
		return;
	}

	const FRotator CurrentRotation = GetControlRotation();
	float DesiredYaw = CurrentRotation.Yaw;

	// No valid focal point (e.g. Move focus clears at the end of a path): keep facing where we are.
	const FVector FocalPoint = GetFocalPoint();
	if (FAISystem::IsValidLocation(FocalPoint))
	{
		DesiredYaw = (FocalPoint - MyPawn->GetPawnViewLocation()).Rotation().Yaw;
	}

	// Enemies stay upright for now (no pitch toward targets above or below).
	FRotator NewRotation(0.f, CurrentRotation.Yaw, 0.f);
	NewRotation.Yaw = MaxYawRateDegreesPerSecond > 0.f
		? FMath::FixedTurn(CurrentRotation.Yaw, DesiredYaw, MaxYawRateDegreesPerSecond * DeltaTime)
		: DesiredYaw;

	SetControlRotation(NewRotation);

	if (bUpdatePawn && !MyPawn->GetActorRotation().Equals(NewRotation, 1e-3f))
	{
		MyPawn->FaceRotation(NewRotation, DeltaTime);
	}
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
