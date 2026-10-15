// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "AtlantisAITypes.h"
#include "StateTreeEvaluatorBase.h"
#include "STEval_PerceptionSys.generated.h"

class AAtlantisPlayerCharacter;
class AAtlantisEnemyController;

// All State Tree elements must contain an instance data struct, regardless of whether it has anything useful.
// Instance Data are required when attempting to emit parameters, but also when consuming data from other
// State Tree Elements or declaring external systems that are required to be accessed by this State Tree Element.

// Why can't we just declare them in the FSTEval_PerceptionSys struct itself? Because ironically, State Tree Elements 
// are themselves Stateless. **DO NOT ATTEMPT TO STORE ANY STATEFUL INFORMATION OUTSIDE OF AN INSTANCE DATA!**
// (Unless you're storing it in some persistent object outside of the state tree system, like a controller)
// This is because the state tree execution context is scrapped and recreated every tick.

// In general, data, parameters, and anything external ought be stored in the Instance Data.
// Any initial parameters, such as, say, the initial duration of timers, maximum distance to a location, etc. are fine
// to store in the actual struct itself.

USTRUCT()
struct PROJECTATLANTIS_API FSTEval_PerceptionSysInstanceData
{
	GENERATED_BODY()
	
	// State Tree parameters with Categories are *not* arbitrary.
	// They affect how users can interact with variables.
	
	// Context-type parameters utilize the State Tree Context, effectively external variables
	// or systems not managed by the State Tree directly so we can use their parameters in our State Tree.
	// Note that any context or other input parameters that are left unassigned in the State Tree Editor
	// will cause the State Tree to not compile!
	// Note that you can't just access every single possible thing under the sun from the Context.
	// The State Tree Context in question has to have explicit permission to access them.
	// Since we're using the bog-standard Unreal-provided State Tree Context as of writing, we have
	// guaranteed access to the using actor and its controller (see StateTreeAIComponentSchema.h and StateTreeComponentSchema.h)
	
	UPROPERTY(EditDefaultsOnly, Category = Context)
	TObjectPtr<AAtlantisEnemyController> AIController;
};

// You may notice that this struct is... well, a struct. Most State Tree Elements are composed of structs and not classes.
// When writing C++ code for State Tree Elements, you may be tempted to derive from StateTree_X_BlueprintBase, but
// unless you have explicit plans to make State Tree Elements that should be derived from to make Blueprint children for,
// use the struct version instead - in this case, FStateTree_X_CommonBase.

/**
 * Uses an Atlantis Enemy's Perception Component in order to reflect perception in our AI.
 */
USTRUCT(DisplayName = "Perception Evaluator")
struct PROJECTATLANTIS_API FSTEval_PerceptionSys : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()
	
	// Required to properly get instance data.
	virtual const UStruct* GetInstanceDataType() const override {return FSTEval_PerceptionSysInstanceData::StaticStruct();}

	void SetEnemyStatus(AAtlantisEnemyController* EnemyController, EMonsterStatus NewStatus) const;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	
	// An example of a parameter that can logically exist in the struct itself, rather than the Instance Data.
	// The Parameter Category denotes a value that should not use a different State Tree Node's output value,
	// and instead use an explicit user-defined value.
	// The distance at which we should instantly enter the combat state if we see a player, crouched or not.
	UPROPERTY(EditDefaultsOnly, Category = Parameter, meta = (Units = "cm"))
	float SightInstantAggroRadius = 1000.f;
	
	// When we lose all stimuli from a player character, how long do we wait until their
	// associated perception value decays?
	UPROPERTY(EditDefaultsOnly, Category = Parameter, meta = (Units = "s", ClampMin = 0, UIMin = 0))
	float InitialPerceptionHysteresisTime = 5.f;

	// How quickly we should "forget" the associated player.
	UPROPERTY(EditDefaultsOnly, Category = Parameter, meta = (ClampMin = 0, UIMin = 0, ClampMax = 1, UIMax = 1))
	float PerceptionDecayPerSecond = 0.1f;
	
	// This Curve Table should depict how much perception value is accrued with distance.
	// Even though both Sight Instant Aggro Radius and the maximum distance to see someone are separate values,
	// I think this curve's X-axis should be purely distance-based, and not from 0-1. This should make it easier for
	// designers to visualize how much perception value is accrued per second at specific distances.
	// Speaking of which, the Y-axis should be the perception value per tick. Perception values exist from 0-1.
	UPROPERTY(EditDefaultsOnly, Category = Parameter)
	TObjectPtr<UCurveFloat> SightDistanceCurve;
};
