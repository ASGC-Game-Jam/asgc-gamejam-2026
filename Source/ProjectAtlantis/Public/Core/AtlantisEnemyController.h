// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisAIController.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisEnemyController.generated.h"

class AAtlantisPlayerCharacter;
class UAtlantisSightConeComponent;

struct FPlayerPerceptionData
{
	float PerceptionValue = 0.f;
	FVector LastStimulusLocation = FVector::ZeroVector;
	
	// If we lose all of this target's stimuli, this tracks the remaining time
	// until we make our Perception Value decay.
	TOptional<float> PerceptionHysteresisTime;
	
	bool bHasBeenProcessedThisTick = false;
	bool bIsAggroed = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMonsterStatusChanged, EMonsterStatus, OldStatus, EMonsterStatus, NewStatus);

UCLASS()
class PROJECTATLANTIS_API AAtlantisEnemyController : public AAtlantisAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAtlantisEnemyController();

	virtual void Tick(float DeltaSeconds) override;
	
	/** Sets the status and broadcasts OnStatusChanged if it actually changed. */
	void SetStatus(EMonsterStatus InStatus);
	
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI")
	EMonsterStatus GetCurrentStatus() const { return CurrentStatus; };
	
	/** Fired whenever this enemy's status changes (e.g. Idle -> Alerted -> Combat). */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|AI")
	FOnMonsterStatusChanged OnStatusChanged;
	
	// Current list of all player characters we know of, whether or not we're fighting them.
	TMap<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData> PerceivedActors;
	
protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;
	
	// Read from Blueprints via GetCurrentStatus().
	UPROPERTY(VisibleInstanceOnly, Category = "Atlantis|AI")
	EMonsterStatus CurrentStatus = EMonsterStatus::Idle;
	
	/** Debug visualizer for the sight sense. We call it every tick; its settings live on the component. */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UAtlantisSightConeComponent> SightCone;
};
