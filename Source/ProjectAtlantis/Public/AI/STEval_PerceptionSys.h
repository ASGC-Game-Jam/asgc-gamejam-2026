// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "StateTreeEvaluatorBase.h"
#include "STEval_PerceptionSys.generated.h"

class AAtlantisEnemyController;

// All State Tree elements must contain an instance data struct, regardless of whether it has anything useful.
// Instance Data are required when attempting to emit parameters, but also when consuming data from other
// State Tree Elements or declaring external systems that are required to be accessed by this State Tree Element.

// Why can't we just declare them in the FSTEval_PerceptionSys struct itself? Because ironically, State Tree Elements 
// are themselves Stateless. **DO NOT ATTEMPT TO STORE ANY STATEFUL INFORMATION OUTSIDE OF AN INSTANCE DATA!**
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
	// Output-type parameters cannot be assigned to, and other State Tree Elements can accept
	// their output as input parameters.
	UPROPERTY(VisibleAnywhere, Category = Output)
	FVector SensedLocation = FAISystem::InvalidLocation;
	
	// Context-type parameters utilize the State Tree Context, effectively external variables
	// or systems not managed by the State Tree directly so we can use their parameters in our State Tree.
	// Note that any context or other input parameters that are left unassigned in the State Tree Editor
	// will cause the State Tree to not compile!
	UPROPERTY(EditDefaultsOnly, Category = Context)
	TObjectPtr<AAtlantisEnemyController> AIController;
};

/**
 * Uses an Atlantis Enemy's Perception Component in order to reflect perception in our AI.
 */
USTRUCT()
struct PROJECTATLANTIS_API FSTEval_PerceptionSys : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()
	
	// Required to properly get instance data.
	virtual const UStruct* GetInstanceDataType() const override {return FSTEval_PerceptionSysInstanceData::StaticStruct();};
	
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
