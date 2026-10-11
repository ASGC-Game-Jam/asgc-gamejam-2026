// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Journal/AtlantisJournalObjectivesSubsystem.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisJournalObjectivesTest,
	"Atlantis.Journal.ObjectivesSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisJournalObjectivesTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UAtlantisJournalObjectivesSubsystem> Journal(NewObject<UAtlantisJournalObjectivesSubsystem>());
	TestEqual(TEXT("Starts empty"), Journal->GetVisibleObjectives().Num(), 0);
	FJournalObjectivePresentation Active;
	Active.ObjectiveId = TEXT("InspectRoom");
	Active.Text = FText::FromString(TEXT("Inspect the room"));
	Active.Context = FText::FromString(TEXT("The crew mentioned unusual noises."));
	Active.Location = FText::FromString(TEXT("Engine room"));
	FJournalObjectivePresentation Completed = Active;
	Completed.ObjectiveId = TEXT("SpeakToCrew");
	Completed.State = EJournalObjectiveDisplayState::Completed;
	FJournalObjectivePresentation Failed = Active;
	Failed.ObjectiveId = TEXT("ReachDoor");
	Failed.State = EJournalObjectiveDisplayState::Failed;
	TestTrue(TEXT("Accept supplied states"), Journal->SubmitObjectiveSnapshot({Active, Completed, Failed}));
	TArray<FJournalObjectivePresentation> Visible = Journal->GetVisibleObjectives();
	TestEqual(TEXT("Default policy shows active only"), Visible.Num(), 1);
	if (Visible.Num() == 1)
	{
		TestEqual(TEXT("Context is exactly owner-provided"), Visible[0].Context.ToString(), Active.Context.ToString());
		TestEqual(TEXT("Location is exactly owner-provided"), Visible[0].Location.ToString(), Active.Location.ToString());
		Visible[0].Text = FText::FromString(TEXT("Mutated consumer copy"));
		TestEqual(TEXT("Consumer cannot mutate cache"), Journal->GetVisibleObjectives()[0].Text.ToString(), Active.Text.ToString());
	}
	Journal->SetHistoryVisibility(true, false);
	TestEqual(TEXT("Completed history is Design-selectable"), Journal->GetVisibleObjectives().Num(), 2);
	Journal->SetHistoryVisibility(false, true);
	Visible = Journal->GetVisibleObjectives();
	TestEqual(TEXT("Failed history is independently selectable"), Visible.Num(), 2);
	if (Visible.Num() == 2)
	{
		TestEqual(TEXT("Owner order preserved"), Visible[1].ObjectiveId, Failed.ObjectiveId);
	}
	Journal->SetHistoryVisibility(true, true);
	TestEqual(TEXT("Filtering retained both history records"), Journal->GetVisibleObjectives().Num(), 3);
	TestFalse(TEXT("Duplicate IDs reject complete replacement"), Journal->SubmitObjectiveSnapshot({Active, Active}));
	TestEqual(TEXT("Rejected snapshot preserves cache"), Journal->GetVisibleObjectives().Num(), 3);
	FJournalObjectivePresentation Invalid = Active;
	Invalid.ObjectiveId = NAME_None;
	TestFalse(TEXT("Missing ID is rejected"), Journal->SubmitObjectiveSnapshot({Invalid}));
	Invalid = Active;
	Invalid.Text = FText::FromString(TEXT("  \t"));
	TestFalse(TEXT("Blank objective is rejected"), Journal->SubmitObjectiveSnapshot({Invalid}));
	Invalid = Active;
	Invalid.State = static_cast<EJournalObjectiveDisplayState>(255);
	TestFalse(TEXT("Unknown state is rejected"), Journal->SubmitObjectiveSnapshot({Invalid}));
	Active.State = EJournalObjectiveDisplayState::Completed;
	Active.Text = FText::FromString(TEXT("Room inspection complete"));
	Active.Context = FText::GetEmpty();
	Active.Location = FText::GetEmpty();
	TestTrue(TEXT("Owner transition and text replacement accepted"), Journal->SubmitObjectiveSnapshot({Active}));
	Visible = Journal->GetVisibleObjectives();
	if (Visible.Num() == 1)
	{
		TestTrue(TEXT("Journal displays owner completion"), Visible[0].State == EJournalObjectiveDisplayState::Completed);
		TestEqual(TEXT("Journal displays updated text"), Visible[0].Text.ToString(), Active.Text.ToString());
		TestTrue(TEXT("Optional fields can be removed"), Visible[0].Context.IsEmpty() && Visible[0].Location.IsEmpty());
	}
	Journal->SetHistoryVisibility(false, false);
	TestEqual(TEXT("Journal does not reactivate owner completion"), Journal->GetVisibleObjectives().Num(), 0);
	TestTrue(TEXT("Empty owner snapshot clears stale objectives"), Journal->SubmitObjectiveSnapshot({}));
	Journal->SetHistoryVisibility(true, true);
	TestEqual(TEXT("Empty snapshot removed history"), Journal->GetVisibleObjectives().Num(), 0);
	Journal->SubmitObjectiveSnapshot({Active});
	Journal->ClearObjectives();
	TestEqual(TEXT("Run reset removes objective presentation"), Journal->GetVisibleObjectives().Num(), 0);
	return true;
}

#endif
