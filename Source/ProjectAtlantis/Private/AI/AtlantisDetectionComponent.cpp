// Copyright (c) 2026 ASGC

#include "AI/AtlantisDetectionComponent.h"

#include "AIController.h"
#include "Core/AtlantisEnemyController.h"
#include "Core/AtlantisHidingComponent.h"
#include "Core/AtlantisPlayerCharacter.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	int32 GAtlantisDetectionDraw = 0;
	FAutoConsoleVariableRef CVarAtlantisDetectionDraw(
		TEXT("Atlantis.AI.Detection.Draw"),
		GAtlantisDetectionDraw,
		TEXT("Draw enemy detection memory. 0 = off, 1 = last known locations (red aggroed / yellow seen / orange lost), search points and a status line above each enemy."),
		ECVF_Cheat);
}

UAtlantisDetectionComponent::UAtlantisDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAtlantisDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	ensureMsgf(Cast<AAIController>(GetOwner()) != nullptr, TEXT("[%hs] %s must be added to an AI controller, not %s."),
		__FUNCTION__, *GetName(), *GetNameSafe(GetOwner()));
}

UAtlantisDetectionComponent* UAtlantisDetectionComponent::FindFor(const AActor* ControllerOrPawn)
{
	if (const APawn* Pawn = Cast<APawn>(ControllerOrPawn))
	{
		ControllerOrPawn = Pawn->GetController();
	}
	return ControllerOrPawn != nullptr ? ControllerOrPawn->FindComponentByClass<UAtlantisDetectionComponent>() : nullptr;
}

FAtlantisDetectionTuning UAtlantisDetectionComponent::MakeTuning() const
{
	FAtlantisDetectionTuning Tuning;
	Tuning.SuspicionThreshold = SuspicionThreshold;
	Tuning.MaxSearchDuration = MaxSearchDuration;
	Tuning.GraceTime = GraceTime;
	Tuning.DecayPerSecond = DecayPerSecond;
	return Tuning;
}

bool UAtlantisDetectionComponent::IsConcealed(const AActor* Target)
{
	const UAtlantisHidingComponent* Hiding = Target != nullptr ? Target->FindComponentByClass<UAtlantisHidingComponent>() : nullptr;
	return Hiding != nullptr && Hiding->IsHiding();
}

UAIPerceptionComponent* UAtlantisDetectionComponent::GetPerception() const
{
	AAIController* Controller = Cast<AAIController>(GetOwner());
	if (Controller == nullptr)
	{
		return nullptr;
	}
	UAIPerceptionComponent* Perception = Controller->GetPerceptionComponent();
	return Perception != nullptr ? Perception : Controller->FindComponentByClass<UAIPerceptionComponent>();
}

void UAtlantisDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateDetection(DeltaTime);
	DrawDebug();
}

void UAtlantisDetectionComponent::DrawDebug() const
{
#if ENABLE_DRAW_DEBUG
	if (GAtlantisDetectionDraw <= 0)
	{
		return;
	}

	const AAIController* Controller = Cast<AAIController>(GetOwner());
	APawn* EnemyPawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
	const UWorld* World = GetWorld();
	if (EnemyPawn == nullptr || World == nullptr)
	{
		return;
	}

	FString Text = FString::Printf(TEXT("%s  meter %.2f"), *UEnum::GetDisplayValueAsText(Status).ToString(), SuspectMeter);
	for (const TPair<TWeakObjectPtr<AActor>, FAtlantisPlayerDetection>& Pair : Players)
	{
		const FAtlantisPlayerDetection& Data = Pair.Value;
		const FColor Color = Data.bIsAggroed ? FColor::Red : (Data.bWasSensedLastTick ? FColor::Yellow : FColor::Orange);
		DrawDebugSphere(World, Data.LastKnownLocation, 40.f, 12, Color, false, -1.f, 0, 2.f);

		Text += FString::Printf(TEXT("\n%s: %s meter %.2f grace %.1f search %.1f"), *GetNameSafe(Pair.Key.Get()),
			Data.bIsAggroed ? TEXT("AGGRO") : (Data.bWasSensedLastTick ? TEXT("seen") : TEXT("lost")),
			Data.Meter, Data.GraceTimeLeft.Get(0.f), Data.SearchTimeLeft);
	}

	if (SearchMemory.Visited.Num() > 0 || !SearchMemory.Origin.IsZero())
	{
		DrawDebugSphere(World, SearchMemory.Origin, 20.f, 8, FColor::Cyan, false, -1.f, 0, 1.f);
		for (const FVector& Point : SearchMemory.Visited)
		{
			DrawDebugSphere(World, Point, 15.f, 8, FColor::Blue, false, -1.f, 0, 1.f);
		}
	}

	// Duration 0 = this frame only (-1 would keep the text forever).
	DrawDebugString(World, FVector(0.f, 0.f, 160.f), Text, EnemyPawn, FColor::White, 0.f, true);
#endif // ENABLE_DRAW_DEBUG
}

void UAtlantisDetectionComponent::UpdateDetection(float DeltaTime)
{
	using namespace Atlantis::AI::Perception;

	const AAIController* Controller = Cast<AAIController>(GetOwner());
	const APawn* EnemyPawn = Controller != nullptr ? Controller->GetPawn() : nullptr;
	if (EnemyPawn == nullptr)
	{
		// Unpossessed or dead: nothing to remember. Only does work on the first tick without a pawn.
		ResetDetection();
		return;
	}

	UAIPerceptionComponent* Perception = GetPerception();
	if (Perception == nullptr || SightDistanceCurve == nullptr)
	{
		if (!bHasLoggedMissingSetup)
		{
			bHasLoggedMissingSetup = true;
			UE_LOG(LogAtlantisAI, Error, TEXT("[%hs] %s on %s: missing %s; detection is off."), __FUNCTION__, *GetName(), *GetNameSafe(GetOwner()),
				Perception == nullptr ? TEXT("an AI Perception component") : TEXT("a Sight Distance Curve"));
		}
		return;
	}

	const FAtlantisDetectionTuning Tuning = MakeTuning();
	const float Now = GetWorld()->GetTimeSeconds();

	// 1) Players seen this tick fill their meter. Hidden players count as unseen and are never added,
	//    so a hidden player can't be (re)discovered. TODO: hearing.
	TSet<const AActor*> SensedThisTick;
	TArray<AActor*> SensedActors;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SensedActors);
	for (AActor* SensedActor : SensedActors)
	{
		if (Cast<AAtlantisPlayerCharacter>(SensedActor) == nullptr || IsConcealed(SensedActor))
		{
			continue;
		}

		FAtlantisDetectionFactors Factors;
		Factors.Distance = GetSightDistance(EnemyPawn->GetActorLocation(), SensedActor->GetActorLocation());
		TickSensed(Players.FindOrAdd(SensedActor), ComputeSightFillRate(Factors, SightDistanceCurve), DeltaTime, SensedActor->GetActorLocation(), Now);
		SensedThisTick.Add(SensedActor);
	}

	// 2) Everyone else: grace time, then drain. 3) Search bookkeeping for all. 4) Forget the empty ones.
	for (auto It = Players.CreateIterator(); It; ++It)
	{
		const bool bSensed = SensedThisTick.Contains(It.Key().Get());
		if (!bSensed)
		{
			TickUnsensed(It.Value(), DeltaTime, Tuning);
		}
		TickSuspicion(It.Value(), bSensed, DeltaTime, Tuning);

		if (!It.Key().IsValid() || ShouldForget(It.Value()))
		{
			It.RemoveCurrent();
		}
	}

#if ENABLE_VISUAL_LOG
	for (const TPair<TWeakObjectPtr<AActor>, FAtlantisPlayerDetection>& Pair : Players)
	{
		const FAtlantisPlayerDetection& Data = Pair.Value;
		UE_VLOG_LOCATION(GetOwner(), LogAtlantisAI, Verbose, Data.LastKnownLocation, 10.f,
			Data.bIsAggroed ? FColor::Red : (Data.bWasSensedLastTick ? FColor::Yellow : FColor::Orange),
			TEXT("%s: meter %.2f, seen %d, grace %.1f, search %.1f"), *GetNameSafe(Pair.Key.Get()), Data.Meter,
			Data.bWasSensedLastTick, Data.GraceTimeLeft.Get(0.f), Data.SearchTimeLeft);
	}
#endif // ENABLE_VISUAL_LOG

	RefreshStatus();
}

void UAtlantisDetectionComponent::RefreshStatus()
{
	using namespace Atlantis::AI::Perception;
	const FAtlantisDetectionTuning Tuning = MakeTuning();

	TArray<const TPair<TWeakObjectPtr<AActor>, FAtlantisPlayerDetection>*> Pairs;
	TArray<const FAtlantisPlayerDetection*> Entries;
	for (const TPair<TWeakObjectPtr<AActor>, FAtlantisPlayerDetection>& Pair : Players)
	{
		Pairs.Add(&Pair);
		Entries.Add(&Pair.Value);
	}

	const int32 TargetIndex = FindSearchTarget(Entries, Tuning);
	Suspect = TargetIndex != INDEX_NONE ? Pairs[TargetIndex]->Key : nullptr;
	SuspectMeter = TargetIndex != INDEX_NONE ? Pairs[TargetIndex]->Value.Meter : 0.f;
	if (TargetIndex != INDEX_NONE)
	{
		SuspectLastKnownLocation = Pairs[TargetIndex]->Value.LastKnownLocation;
	}

	const EMonsterStatus NewStatus = ComputeMonsterStatus(Entries, Tuning);
	if (NewStatus != Status)
	{
		const EMonsterStatus OldStatus = Status;
		Status = NewStatus;
		UE_VLOG(GetOwner(), LogAtlantisAI, Log, TEXT("Detection status %s -> %s"), *UEnum::GetValueAsString(OldStatus), *UEnum::GetValueAsString(NewStatus));
		OnStatusChanged.Broadcast(OldStatus, NewStatus);
	}

	// Existing enemy controller drives the sight cone colour, its status event and the detection marker.
	if (AAtlantisEnemyController* EnemyController = Cast<AAtlantisEnemyController>(GetOwner()))
	{
		EnemyController->SetStatus(Status);
		EnemyController->SetDetection(SuspectMeter);
	}
}

bool UAtlantisDetectionComponent::GetLastKnownLocation(FVector& OutLocation) const
{
	if (!Suspect.IsValid())
	{
		return false;
	}
	OutLocation = SuspectLastKnownLocation;
	return true;
}

void UAtlantisDetectionComponent::ForgetLostTargets()
{
	Atlantis::AI::Perception::ForgetUnsensed(Players);
	RefreshStatus();
}

void UAtlantisDetectionComponent::ResetDetection()
{
	const bool bHasAnythingToForget = !Players.IsEmpty() || Status != EMonsterStatus::Idle || Suspect.IsValid()
		|| SuspectMeter > 0.f || !SearchMemory.Visited.IsEmpty() || !SearchMemory.Origin.IsZero();
	if (!bHasAnythingToForget)
	{
		return;
	}

	Players.Reset();
	SearchMemory = FAtlantisSearchMemory();
	SuspectLastKnownLocation = FVector::ZeroVector;
	RefreshStatus();
}
