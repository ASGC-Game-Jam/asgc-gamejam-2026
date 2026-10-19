// Copyright (c) 2026 ASGC
#include "Core/AtlantisPlayerWaterPostProcess.h"
#include "Components/PostProcessComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"
#include "SceneView.h"

AAtlantisPlayerWaterPostProcess::AAtlantisPlayerWaterPostProcess()
{
	PrimaryActorTick.bCanEverTick = false;
	PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PlayerWaterPostProcess"));
	SetRootComponent(PostProcess);
	PostProcess->bUnbound = true;
	PostProcess->Settings.WeightedBlendables.Array.Add(FWeightedBlendable(1.f, this));
}

APawn* AAtlantisPlayerWaterPostProcess::ResolveViewPawn(const FSceneView& View) const
{
	UWorld* World = GetWorld();
	UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
	if (!World || !World->IsGameWorld() || !Instance || View.PlayerIndex == INDEX_NONE) { return nullptr; }
	// PlayerIndex is the LocalPlayer controller ID (LocalPlayer.cpp), not the view target actor.
	// Dialogue and third-person camera targets therefore do not replace the player's water state.
	for (ULocalPlayer* Player : Instance->GetLocalPlayers())
	{
		if (Player && Player->GetControllerId() == View.PlayerIndex)
		{
			APlayerController* Controller = Player->GetPlayerController(World);
			return Controller ? Controller->GetPawn() : nullptr;
		}
	}
	return nullptr;
}

float AAtlantisPlayerWaterPostProcess::GetPlayerBlendWeight(const APawn* Pawn) const
{
	if (!Pawn || Pawn->GetWorld() != GetWorld() || !IsValid(SourceVolume) || !SourceVolume->bEnabled) { return 0.f; }
	const APhysicsVolume* Water = Pawn->GetPhysicsVolume();
	if (!Water || !Water->bWaterVolume) { return 0.f; }
	float Distance = -1.f;
	const float Radius = FMath::Max(0.f, SourceVolume->BlendRadius);
	SourceVolume->EncompassesPoint(Pawn->GetActorLocation(), 0.f, &Distance);
	if (!FMath::IsFinite(Distance) || Distance < 0.f || Distance > Radius) { return 0.f; }
	const float SpatialWeight = Radius > 0.f ? 1.f - FMath::Clamp(Distance / Radius, 0.f, 1.f) : (Distance <= 0.f ? 1.f : 0.f);
	return FMath::Clamp(MaterialWeight, 0.f, 1.f) * FMath::Clamp(SourceVolume->BlendWeight, 0.f, 1.f) * SpatialWeight;
}

void AAtlantisPlayerWaterPostProcess::OverrideBlendableSettings(FSceneView& View, float Weight) const
{
	if (!IsValid(UnderwaterMaterial)) { return; }
	const float PlayerWeight = FMath::Clamp(Weight, 0.f, 1.f) * GetPlayerBlendWeight(ResolveViewPawn(View));
	if (PlayerWeight > 0.f) { UnderwaterMaterial->OverrideBlendableSettings(View, PlayerWeight); }
}
