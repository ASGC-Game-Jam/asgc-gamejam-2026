// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisAIController.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisEnemyController.generated.h"

class AAtlantisPlayerCharacter;

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

UCLASS()
class PROJECTATLANTIS_API AAtlantisEnemyController : public AAtlantisAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAtlantisEnemyController();

	void SetStatus(EMonsterStatus InStatus) { CurrentStatus = InStatus; };
	
	EMonsterStatus GetCurrentStatus() const { return CurrentStatus; };
	
	// Current list of all player characters we know of, whether or not we're fighting them.
	TMap<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData> PerceivedActors;
	
protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;
	
	EMonsterStatus CurrentStatus = EMonsterStatus::Idle;
};
