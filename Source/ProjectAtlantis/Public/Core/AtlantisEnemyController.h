// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisAIController.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisEnemyController.generated.h"

UCLASS()
class PROJECTATLANTIS_API AAtlantisEnemyController : public AAtlantisAIController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AAtlantisEnemyController();

	void SetStatus(EMonsterStatus InStatus) { CurrentStatus = InStatus; };
	
	EMonsterStatus GetCurrentStatus() const { return CurrentStatus; };
	
protected:
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Atlantis|AI")
	TObjectPtr<UAIPerceptionComponent> AIPerception;
	
	EMonsterStatus CurrentStatus = EMonsterStatus::Idle;
};
