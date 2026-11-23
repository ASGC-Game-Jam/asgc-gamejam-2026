// Copyright (c) 2026 ASGC

#include "AI/Detection/AtlantisSightConeComponent.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"

UAtlantisSightConeComponent::UAtlantisSightConeComponent()
{
	// The owner drives drawing, so this component never ticks itself.
	PrimaryComponentTick.bCanEverTick = false;
}

void UAtlantisSightConeComponent::DrawSightCone(const UAIPerceptionComponent* PerceptionComponent, const APawn* ViewPawn, EMonsterStatus Status) const
{
#if ENABLE_DRAW_DEBUG
	if (bDrawSightCone == false || PerceptionComponent == nullptr || ViewPawn == nullptr)
	{
		return;
	}

	// Read the real sight config so the cone always matches what the AI can actually see.
	const UAISenseConfig_Sight* SightConfig = Cast<UAISenseConfig_Sight>(
		PerceptionComponent->GetSenseConfig(UAISense::GetSenseID<UAISense_Sight>()));
	if (SightConfig == nullptr)
	{
		return;
	}

	// Same viewpoint the sight sense uses.
	FVector EyeLocation;
	FRotator EyeRotation;
	ViewPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);

	const float HalfAngleRad = FMath::DegreesToRadians(SightConfig->PeripheralVisionAngleDegrees);
	const FVector EyeDirection = EyeRotation.Vector();
	const FColor ConeColor = GetColorForStatus(Status);

	// Main cone: where the AI can first spot a target.
	DrawDebugCone(GetWorld(), EyeLocation, EyeDirection, SightConfig->SightRadius,
		HalfAngleRad, HalfAngleRad, NumSides, ConeColor, false, -1.f, 0, LineThickness);

	// Outer cone: how far a target that's already been seen stays seen.
	if (bDrawLoseSightCone && SightConfig->LoseSightRadius > SightConfig->SightRadius)
	{
		FColor LoseSightColor = (FLinearColor(ConeColor) * LoseSightConeBrightness).ToFColor(true);
		LoseSightColor.A = ConeColor.A; // Dim the RGB only; scaling would also fade alpha.
		DrawDebugCone(GetWorld(), EyeLocation, EyeDirection, SightConfig->LoseSightRadius,
			HalfAngleRad, HalfAngleRad, NumSides, LoseSightColor, false, -1.f, 0, LineThickness * 0.5f);
	}
#endif // ENABLE_DRAW_DEBUG
}

FColor UAtlantisSightConeComponent::GetColorForStatus(EMonsterStatus Status) const
{
	switch (Status)
	{
	case EMonsterStatus::Alerted:
	case EMonsterStatus::Searching:
		return SuspiciousColor;
	case EMonsterStatus::Combat:
		return DetectedColor;
	default:
		return IdleColor;
	}
}
