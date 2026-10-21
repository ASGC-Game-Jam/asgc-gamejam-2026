// Copyright (c) 2026 ASGC

#include "AI/StateTreePerceptionEvaluator.h"

#include "StateTreeExecutionContext.h"
#include "Core/AtlantisAICharacter.h"
#include "Core/AtlantisEnemyController.h"
#include "Core/AtlantisPlayerCharacter.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

void FStateTreePerceptionEvaluator::SetEnemyStatus(AAtlantisEnemyController& EnemyController, EMonsterStatus NewStatus) const
{
	EMonsterStatus CurrentStatus = EnemyController.GetCurrentStatus();
	if (CurrentStatus != NewStatus)
	{
		EnemyController.SetStatus(NewStatus);
	}
}

void FStateTreePerceptionEvaluator::ProcessSensedActor(const float DeltaTime, AAtlantisEnemyController& EnemyController, AActor* SensedActor) const
{
	const AAtlantisAICharacter* EnemyPawn = Cast<AAtlantisAICharacter>(EnemyController.GetCharacter());
	AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(SensedActor);
	if (PlayerCharacter == nullptr)
	{
		return;
	}

	FPlayerPerceptionData& PlayerPerceptionData = EnemyController.PerceivedActors.FindOrAdd(PlayerCharacter);
		
	// Skip processing actors we're already aggro'd on. 
	if (PlayerPerceptionData.bIsAggroed)
	{
		PlayerPerceptionData.bHasBeenProcessedThisTick = true;
		return;
	}
		
	float DistanceToSensedActor = FVector::Dist2D(EnemyPawn->GetActorLocation(), SensedActor->GetActorLocation());
		
	// If we have a sensed actor that is within our instant aggro radius, instantly engage in combat.
	if (DistanceToSensedActor < SightInstantAggroRadius)
	{
		UE_LOG(LogStateTree,
		       VeryVerbose,
		       TEXT("[%hs] Player Character %s is within %.2f units of enemy character %s. Instantly aggroing."),
		       __FUNCTION__,
		       *PlayerCharacter->GetName(),
		       SightInstantAggroRadius,
		       *EnemyPawn->GetName()
		);
			
		PlayerPerceptionData.PerceptionValue = 1.f;
		PlayerPerceptionData.bIsAggroed = true;
	}
		
	// Otherwise, we need to accumulate perception value, scaled to the distance between the enemy and player.
	else
	{
		float ScaledPerceptionValue = SightDistanceCurve->GetFloatValue(DistanceToSensedActor) * DeltaTime;
			
		if (PlayerPerceptionData.PerceptionValue + ScaledPerceptionValue > 1.f)
		{
			PlayerPerceptionData.PerceptionValue = 1.f;
			PlayerPerceptionData.bIsAggroed = true;
				
			UE_LOG(LogStateTree,
			       Verbose,
			       TEXT("[%hs] Enemy Character %s has perception value %.2f on Player Character %s. Aggroing."),
			       __FUNCTION__,
			       *EnemyPawn->GetName(),
			       PlayerPerceptionData.PerceptionValue,
			       *PlayerCharacter->GetName()
			);
		}
		else
		{
			UE_LOG(LogStateTree,
			       Verbose,
			       TEXT("[%hs] Enemy Character %s has perception value %.2f on Player Character %s, adding %.2f this tick."),
			       __FUNCTION__,
			       *EnemyPawn->GetName(),
			       PlayerPerceptionData.PerceptionValue,
			       *PlayerCharacter->GetName(),
			       ScaledPerceptionValue
			);

			PlayerPerceptionData.PerceptionValue += ScaledPerceptionValue;
		}
	}
		
	PlayerPerceptionData.bHasBeenProcessedThisTick = true;
	PlayerPerceptionData.LastStimulusLocation = PlayerCharacter->GetActorLocation();
	PlayerPerceptionData.PerceptionHysteresisTime.Reset();
}

void FStateTreePerceptionEvaluator::ProcessUnsensedActor(const float DeltaTime,
	const AAtlantisAICharacter* EnemyPawn,
	TPair<TObjectPtr<AAtlantisPlayerCharacter>,
	FPlayerPerceptionData>& PerceptionData) const
{
	if (PerceptionData.Value.bHasBeenProcessedThisTick)
	{
		return;
	}
		
	TOptional<float>& PerceptionHysteresisTime = PerceptionData.Value.PerceptionHysteresisTime;
		
	// Case 1: We have our hysteresis time set. Decrement it.
	if (PerceptionHysteresisTime.IsSet() && PerceptionHysteresisTime.GetValue() > 0.f)
	{
		PerceptionHysteresisTime.GetValue() -= DeltaTime;
			
		float DecayedPerceptionValue = 0.f;
			
		// If we run out of hysteresis time, decay our perception value by the remaining time.
		if (PerceptionHysteresisTime.GetValue() <= 0.f)
		{
			float RemainingTime = PerceptionHysteresisTime.GetValue() * -1.f;

			// Do not reset the hysteresis time here. Only if we perceive the actor again should we reset it.
				
			DecayedPerceptionValue = RemainingTime * DeltaTime;
				
			PerceptionData.Value.PerceptionValue -= DecayedPerceptionValue;
				
			// If we are also aggro'd onto this actor, we need to unset its aggro status
			PerceptionData.Value.bIsAggroed = false;
		}
			
		UE_LOG(LogStateTree,
		       Verbose,
		       TEXT("[%hs] Character: %s, Player Character: %s, Perception Value: %.2f, Hysteresis Time: %.2f. (Removed %.2f Perception this tick)"),
		       __FUNCTION__,
		       *EnemyPawn->GetName(),
		       *PerceptionData.Key->GetName(),
		       PerceptionData.Value.PerceptionValue,
		       PerceptionData.Value.PerceptionHysteresisTime.Get(0.f),
		       DecayedPerceptionValue
		);

	}
	// Case 2: Our hysteresis time is unset. Set it.
	else if (PerceptionData.Value.PerceptionHysteresisTime.IsSet() == false)
	{
		PerceptionData.Value.PerceptionHysteresisTime = InitialPerceptionHysteresisTime - DeltaTime;
			
		UE_LOG(LogStateTree,
		       Verbose,
		       TEXT("[%hs] Character: %s, Player Character: %s, Perception Value: %.2f, Hysteresis Time: %.2f."),
		       __FUNCTION__,
		       *EnemyPawn->GetName(),
		       *PerceptionData.Key->GetName(),
		       PerceptionData.Value.PerceptionValue,
		       PerceptionData.Value.PerceptionHysteresisTime.Get(0.f)
		);

	}
	// Case 3: We have run out of hysteresis time. Decay our perception value. 
	else
	{
		float DecayedPerceptionValue = PerceptionDecayPerSecond * DeltaTime;
		PerceptionData.Value.PerceptionValue -= DecayedPerceptionValue;
			
		UE_LOG(LogStateTree,
		       Verbose,
		       TEXT("[%hs] Character: %s, Player Character: %s, Perception Value: %.2f, Removed %.2f Perception this tick."),
		       __FUNCTION__,
		       *EnemyPawn->GetName(),
		       *PerceptionData.Key->GetName(),
		       PerceptionData.Value.PerceptionValue,
		       DecayedPerceptionValue
		);

	}
}

void FStateTreePerceptionEvaluator::HandleStatus(AAtlantisEnemyController* EnemyController, TMap<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData>& PerceivedActors) const
{
	// Change our status depending on a few factors...
	TMap<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData> AggroedActors = PerceivedActors.FilterByPredicate([](auto& PerceivedActor)
	{
		return PerceivedActor.Value.bIsAggroed;
	});
	
	// If we have any actors we need to aggro onto, set us up for combat.
	if (AggroedActors.IsEmpty() == false)
	{
		SetEnemyStatus(*EnemyController, EMonsterStatus::Combat);
	}
	
	// Otherwise, if we're only aware of a player, we're alert.
	if (PerceivedActors.IsEmpty() == false)
	{
		SetEnemyStatus(*EnemyController, EMonsterStatus::Alerted);
	}
	
	// If we have absolutely 0 perceived actors, return to idle.
	else
	{
		SetEnemyStatus(*EnemyController, EMonsterStatus::Idle);
	}
}

void FStateTreePerceptionEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	Super::Tick(Context, DeltaTime);
	
	FStateTreePerceptionEvaluatorInstanceData& InstanceData = Context.GetInstanceData<FStateTreePerceptionEvaluatorInstanceData>(*this);
	AAtlantisEnemyController* EnemyController = InstanceData.AIController.Get();
	if (EnemyController == nullptr)
	{
		UE_LOG(LogStateTree, Error, TEXT("[%hs] Failed to retrieve the AI Controller! (Is it set in the element's components?)"), __FUNCTION__);
		return;
	}
	
	UAIPerceptionComponent* PerceptionComponent = EnemyController->GetPerceptionComponent();
	if (PerceptionComponent == nullptr)
	{
		UE_LOG(LogStateTree, Error, TEXT("[%hs] Failed to retrieve the Perception Component! (Does the operating actor have one in its controller?)"), __FUNCTION__);
		return;
	}
	
	if (SightDistanceCurve == nullptr)
	{
		UE_LOG(LogStateTree, Error, TEXT("[%hs] There is no Sight Distance Curve assigned! (Add one to the Perception Sys Evaluator!)"), __FUNCTION__);
		return;
	}
	
	// Context: We can't assume that there will only be one player character in this game.
	// Therefore, we have to overengineer this perception algorithm to allow for multiple player characters.
	// Keep track of all perceived actors in the enemy controller, tracking their perception values & other relevant values.
	TMap<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData>& PerceivedActors = EnemyController->PerceivedActors;
	for (TPair<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData>& PerceptionData : PerceivedActors)
	{
		// Reset the processing flag.
		PerceptionData.Value.bHasBeenProcessedThisTick = false;
	}
	
	const AAtlantisAICharacter* EnemyPawn = EnemyController->GetPawn<AAtlantisAICharacter>();
	
	UE_CVLOG_CIRCLE(EnemyPawn != nullptr,
		EnemyController,
		LogStateTree,
		Verbose,
		EnemyPawn->GetActorLocation(),
		FVector::UpVector,
		SightInstantAggroRadius,
		FColor::Red,
		TEXT("Instant Sight Radius"));
	
	// TODO: This is just Sight for now. Hearing will be later.
	TArray<AActor*> OutSensedActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), OutSensedActors);
	
	for (auto* SensedActor : OutSensedActors)
	{
		ProcessSensedActor(DeltaTime, *EnemyController, SensedActor);
	}
	
	// Now, process the other actors we did not perceive this tick.
	for (TPair<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData>& PerceptionData : PerceivedActors)
	{
		ProcessUnsensedActor(DeltaTime, EnemyPawn, PerceptionData);
	}
	
#if ENABLE_VISUAL_LOG
	for (TPair<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData>& PerceptionData : PerceivedActors)
	{
		UE_VLOG_LOCATION(EnemyController,
			LogStateTree,
			Verbose,
			PerceptionData.Key->GetActorLocation(),
			5.f,
			PerceptionData.Value.bIsAggroed ? FColor::Red : FColor::Cyan,
			TEXT("Perception Value: %.2f. Hysteresis Time: %.2f."),
			PerceptionData.Value.PerceptionValue,
			PerceptionData.Value.PerceptionHysteresisTime.Get(0.f)
		);
	}
#endif // ENABLE_VISUAL_LOG
	
	// If perception value is 0, remove that perceived actor.
	PerceivedActors = PerceivedActors.FilterByPredicate([](auto& PerceivedActor)
	{
		return PerceivedActor.Value.PerceptionValue > 0;
	});
	
	HandleStatus(EnemyController, PerceivedActors);
}
