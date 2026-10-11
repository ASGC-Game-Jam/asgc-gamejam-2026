// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AtlantisJournalObjectivesSubsystem.generated.h"

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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnJournalObjectivesChanged);

/** Presentation adapter for Objective State & Progression, not a quest state system. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalObjectivesSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Replace the complete disclosed snapshot in owner-specified display order. Invalid input is rejected atomically. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Objectives")
	bool SubmitObjectiveSnapshot(const TArray<FJournalObjectivePresentation>& Snapshot);

	/** Presentation policy only; hidden history stays cached so changing policy does not lose owner data. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Objectives")
	void SetHistoryVisibility(bool bShowCompleted, bool bShowFailed);

	/** Return a copy of visible presentation data without granting state mutation access. */
	UFUNCTION(BlueprintPure, Category = "Journal|Objectives")
	TArray<FJournalObjectivePresentation> GetVisibleObjectives() const;

	/** Clear presentation when the owning game starts a new run or returns to the menu. */
	UFUNCTION(BlueprintCallable, Category = "Journal|Objectives")
	void ClearObjectives();

	UPROPERTY(BlueprintAssignable, Category = "Events|Journal")
	FOnJournalObjectivesChanged OnObjectivesChanged;

private:
	UPROPERTY()
	TArray<FJournalObjectivePresentation> Objectives;

	bool bCompletedVisible = false;
	bool bFailedVisible = false;
};
