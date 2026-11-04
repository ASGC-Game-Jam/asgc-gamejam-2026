// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "AtlantisJournalObjectiveTypes.generated.h"

/** Display state supplied by the objective owner. Journal never computes transitions. */
UENUM(BlueprintType)
enum class EJournalObjectiveDisplayState : uint8
{
	Active,
	Completed,
	Failed
};

/** A disclosed, player-facing snapshot; do not submit internal solution or progression data. */
USTRUCT(BlueprintType)
struct PROJECTATLANTIS_API FJournalObjectivePresentation
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Objectives")
	FName ObjectiveId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Objectives")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Objectives")
	FText Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Objectives")
	FText Location;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Journal|Objectives")
	EJournalObjectiveDisplayState State = EJournalObjectiveDisplayState::Active;
};

