// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisAIController.h"
#include "AtlantisEnemyController.generated.h"

UCLASS()
class PROJECTATLANTIS_API AAtlantisEnemyController : public AAtlantisAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAtlantisEnemyController();
	
protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

};
