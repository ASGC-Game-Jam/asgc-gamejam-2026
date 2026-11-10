// Copyright (c) 2026 ASGC

#pragma once
#include "AIController.h"

#include "AtlantisAIController.generated.h"

class UAIPerceptionComponent;
class UStateTreeAIComponent;

UCLASS()
class PROJECTATLANTIS_API AAtlantisAIController : public AAIController
{
	GENERATED_BODY()
public:
	// Keeps the default argument so subclasses with default constructors still compile.
	// Replaces the engine path following with UAtlantisPathFollowingComponent (3D reach tests for swimming).
	AAtlantisAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
