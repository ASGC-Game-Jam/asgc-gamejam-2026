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
	// We replace the engine's 2D-ish, check for arrival, for our own that checks in fully 3D
	AAtlantisAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
