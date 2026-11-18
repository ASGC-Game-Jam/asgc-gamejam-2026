// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "AI/AtlantisAITypes.h"
#include "AtlantisStateTreeDetectionEvaluator.generated.h"

class AAIController;
class UAtlantisDetectionComponent;

USTRUCT()
struct PROJECTATLANTIS_API FAtlantisStateTreeDetectionEvaluatorInstanceData
{
	GENERATED_BODY()

	/** The enemy's AI controller (bound automatically from the tree context). */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> AIController = nullptr;

	/** The enemy's current status. Idle until detection says otherwise. */
	UPROPERTY(VisibleAnywhere, Category = Output)
	EMonsterStatus Status = EMonsterStatus::Idle;

	/** Where the suspect was last seen. Only meaningful when SuspectActor is set. */
	UPROPERTY(VisibleAnywhere, Category = Output)
	FVector LastKnownLocation = FVector::ZeroVector;

	/** The player the enemy is reacting to (aggroed first, then seen & suspicious, then lost & searched for), or null. */
	UPROPERTY(VisibleAnywhere, Category = Output)
	TObjectPtr<AActor> SuspectActor = nullptr;

	/** Detection meter (0-1) of SuspectActor. */
	UPROPERTY(VisibleAnywhere, Category = Output)
	float Suspicion = 0.f;

	TWeakObjectPtr<UAtlantisDetectionComponent> Detection;
};

/**
 * Exposes the controller's UAtlantisDetectionComponent to the State Tree (status, suspect, last known location, meter).
 * Holds no detection logic itself: the component ticks on its own. Outputs fall back to Idle when it's missing.
 */
USTRUCT(meta = (DisplayName = "Atlantis Detection", Category = "Atlantis|AI"))
struct PROJECTATLANTIS_API FAtlantisStateTreeDetectionEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAtlantisStateTreeDetectionEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void Refresh(FInstanceDataType& InstanceData) const;
};
