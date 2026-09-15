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
	AAtlantisAIController();

protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeComponent;
};
