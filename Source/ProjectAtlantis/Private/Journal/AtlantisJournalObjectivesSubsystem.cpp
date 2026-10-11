// Copyright (c) 2026 ASGC

#include "Journal/AtlantisJournalObjectivesSubsystem.h"
#include "Journal/AtlantisJournalSubsystem.h"
#include "Engine/GameInstance.h"

void UAtlantisJournalObjectivesSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Collection.InitializeDependency<UAtlantisJournalSubsystem>();
	GetGameInstance()->GetSubsystem<UAtlantisJournalSubsystem>()->OnJournalCleared.AddDynamic(this, &ThisClass::ClearObjectives);
}

void UAtlantisJournalObjectivesSubsystem::Deinitialize()
{
	if (UAtlantisJournalSubsystem* Journal = GetGameInstance()->GetSubsystem<UAtlantisJournalSubsystem>())
	{
		Journal->OnJournalCleared.RemoveDynamic(this, &ThisClass::ClearObjectives);
	}
	Objectives.Reset();
	Super::Deinitialize();
}

bool UAtlantisJournalObjectivesSubsystem::SubmitObjectiveSnapshot(const TArray<FJournalObjectivePresentation>& Snapshot)
{
	TSet<FName> Ids;
	for (const FJournalObjectivePresentation& Objective : Snapshot)
	{
		if (Objective.ObjectiveId.IsNone() || Ids.Contains(Objective.ObjectiveId) || Objective.Text.IsEmptyOrWhitespace()
			|| (Objective.State != EJournalObjectiveDisplayState::Active
				&& Objective.State != EJournalObjectiveDisplayState::Completed
				&& Objective.State != EJournalObjectiveDisplayState::Failed))
		{
			return false;
		}
		Ids.Add(Objective.ObjectiveId);
	}
	Objectives = Snapshot;
	OnObjectivesChanged.Broadcast();
	return true;
}

void UAtlantisJournalObjectivesSubsystem::SetHistoryVisibility(bool bShowCompleted, bool bShowFailed)
{
	if (bCompletedVisible != bShowCompleted || bFailedVisible != bShowFailed)
	{
		bCompletedVisible = bShowCompleted;
		bFailedVisible = bShowFailed;
		OnObjectivesChanged.Broadcast();
	}
}

TArray<FJournalObjectivePresentation> UAtlantisJournalObjectivesSubsystem::GetVisibleObjectives() const
{
	TArray<FJournalObjectivePresentation> Visible;
	for (const FJournalObjectivePresentation& Objective : Objectives)
	{
		if (Objective.State == EJournalObjectiveDisplayState::Active
			|| (Objective.State == EJournalObjectiveDisplayState::Completed && bCompletedVisible)
			|| (Objective.State == EJournalObjectiveDisplayState::Failed && bFailedVisible))
		{
			Visible.Add(Objective);
		}
	}
	return Visible;
}

void UAtlantisJournalObjectivesSubsystem::ClearObjectives()
{
	Objectives.Reset();
	OnObjectivesChanged.Broadcast();
}
