// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Journal/AtlantisJournalSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(FAtlantisJournalObjectivesSpec, "Atlantis.Journal.ObjectivesSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FAtlantisJournalObjectivesSpec)

void FAtlantisJournalObjectivesSpec::Define()
{
	It(TEXT("should preserve owner snapshots and independently filter history without mutating gameplay state"), [this]()
	{

		TStrongObjectPtr<UGameInstance> SnapshotOwner(NewObject<UGameInstance>());
		TStrongObjectPtr<UAtlantisJournalSubsystem> Journal(NewObject<UAtlantisJournalSubsystem>(SnapshotOwner.Get()));
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
	});
}


namespace
{
	bool ContainsObjectiveText(UUserWidget* Widget, const FString& Text)
	{
		bool bFound = false;
		Widget->WidgetTree->ForEachWidget([&](UWidget* Child)
		{
			if (!Child->IsVisible()) { return; }
			if (const UTextBlock* Label = Cast<UTextBlock>(Child))
			{
				bFound |= Label->GetText().ToString().Contains(Text);
				return;
			}
			if (UUserWidget* Row = Cast<UUserWidget>(Child))
			{
				bFound |= ContainsObjectiveText(Row, Text);
			}
		});
		return bFound;
	}
}

BEGIN_DEFINE_SPEC(FAtlantisJournalObjectivesViewSpec, "Atlantis.Journal.ObjectivesViewUpdates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UGameInstance> GameInstance;
	UWorld* World = nullptr;
	UAtlantisJournalSubsystem* Journal = nullptr;
	UUserWidget* Widget = nullptr;
	TSharedPtr<SWidget> SlateWidget;
END_DEFINE_SPEC(FAtlantisJournalObjectivesViewSpec)

void FAtlantisJournalObjectivesViewSpec::Define()
{
	BeforeEach([this]()
	{
		GameInstance.Reset(NewObject<UGameInstance>(GEngine));
		GameInstance->InitializeStandalone();
		World = GameInstance->GetWorld();
		Journal = GameInstance->GetSubsystem<UAtlantisJournalSubsystem>();
		UClass* ViewClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ProjectAtlantis/UI/WBP_JournalObjectivesTab.WBP_JournalObjectivesTab_C"));
		Widget = ViewClass ? CreateWidget<UUserWidget>(GameInstance.Get(), ViewClass) : nullptr;
		if (!TestNotNull(TEXT("Authored Objectives tab loads"), Widget)) { return; }
		TestEqual(TEXT("Tab derives directly from UserWidget"), ViewClass->GetSuperClass(), UUserWidget::StaticClass());
		SlateWidget = Widget->TakeWidget();
	});
	AfterEach([this]()
	{
		SlateWidget.Reset();
		if (Widget) { Widget->ReleaseSlateResources(true); }
		GameInstance->Shutdown();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		Widget = nullptr;
		Journal = nullptr;
		World = nullptr;
		GameInstance.Reset();
	});

	Describe(TEXT("Blueprint presentation"), [this]()
	{
		It(TEXT("should refresh authored rows and empty state from Journal updates and reset"), [this]()
		{
			if (!Widget) { return; }
			TestTrue(TEXT("Opening empty view renders empty state"), ContainsObjectiveText(Widget, TEXT("No objectives")));
			FJournalObjectivePresentation Objective;
			Objective.ObjectiveId = TEXT("ReadNote");
			Objective.Text = FText::FromString(TEXT("Read the crew note"));
			Objective.Context = FText::FromString(TEXT("The crew left a record."));
			Objective.Location = FText::FromString(TEXT("Engine room"));
			Journal->SubmitObjectiveSnapshot({Objective});
			TestTrue(TEXT("Owner update refreshes an open view"), ContainsObjectiveText(Widget, TEXT("Active: Read the crew note")));
			TestTrue(TEXT("Context remains owner supplied"), ContainsObjectiveText(Widget, TEXT("The crew left a record.")));
			TestTrue(TEXT("Location is formatted by the Blueprint row"), ContainsObjectiveText(Widget, TEXT("Location: Engine room")));
			TestFalse(TEXT("Populated view hides authored fallback"), ContainsObjectiveText(Widget, TEXT("No objectives")));
			auto* List = CastChecked<UPanelWidget>(Widget->GetWidgetFromName(TEXT("ObjectiveList")));
			TestEqual(TEXT("One row for each visible objective"), List->GetChildrenCount(), 1);
			auto* Row = CastChecked<UUserWidget>(List->GetChildAt(0));
			TestEqual(TEXT("Row derives directly from UserWidget"), Row->GetClass()->GetSuperClass(), UUserWidget::StaticClass());
			Objective.Context = FText::FromString(TEXT("  \t"));
			Objective.Location = FText::GetEmpty();
			Objective.Text = FText::FromString(TEXT("Crew note read"));
			Objective.State = EJournalObjectiveDisplayState::Completed;
			Journal->SubmitObjectiveSnapshot({Objective});
			TestFalse(TEXT("History policy hides completion by default"), ContainsObjectiveText(Widget, TEXT("Crew note read")));
			Journal->SetHistoryVisibility(true, false);
			TestTrue(TEXT("Policy update displays completed text"), ContainsObjectiveText(Widget, TEXT("Completed: Crew note read")));
			Row = CastChecked<UUserWidget>(List->GetChildAt(0));
			TestFalse(TEXT("Whitespace context is collapsed"), Row->GetWidgetFromName(TEXT("ContextText"))->IsVisible());
			TestFalse(TEXT("Empty location is collapsed"), Row->GetWidgetFromName(TEXT("LocationText"))->IsVisible());
			auto* Title = CastChecked<UTextBlock>(Row->GetWidgetFromName(TEXT("ObjectiveText")));
			TestTrue(TEXT("Completed row uses authored completion color"), Title->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor(0.55f, 0.9f, 0.65f)));
			Objective.State = EJournalObjectiveDisplayState::Failed;
			Journal->SetHistoryVisibility(false, true);
			Journal->SubmitObjectiveSnapshot({Objective});
			TestTrue(TEXT("Failed state is formatted by Blueprint"), ContainsObjectiveText(Widget, TEXT("Failed: Crew note read")));
			Row = CastChecked<UUserWidget>(List->GetChildAt(0));
			Title = CastChecked<UTextBlock>(Row->GetWidgetFromName(TEXT("ObjectiveText")));
			TestTrue(TEXT("Failed row uses authored failure color"), Title->GetColorAndOpacity().GetSpecifiedColor().Equals(FLinearColor(1.f, 0.6f, 0.55f)));
			Journal->ClearAll();
			TestEqual(TEXT("ClearAll removes old rows"), List->GetChildrenCount(), 0);
			TestTrue(TEXT("ClearAll restores authored fallback"), ContainsObjectiveText(Widget, TEXT("No objectives")));
		});

		It(TEXT("should unbind its own update event on destruct and refresh once after reconstruction"), [this]()
		{
			if (!Widget) { return; }
			TestEqual(TEXT("Construct binds exactly one listener"), Journal->OnObjectivesChanged.GetAllObjects().Num(), 1);
			SlateWidget.Reset();
			Widget->ReleaseSlateResources(true);
			TestEqual(TEXT("Destruct removes its listener"), Journal->OnObjectivesChanged.GetAllObjects().Num(), 0);
			FJournalObjectivePresentation Objective;
			Objective.ObjectiveId = TEXT("WhileClosed");
			Objective.Text = FText::FromString(TEXT("Latest owner snapshot"));
			Journal->SubmitObjectiveSnapshot({Objective});
			SlateWidget = Widget->TakeWidget();
			TestEqual(TEXT("Reconstruct binds exactly one listener"), Journal->OnObjectivesChanged.GetAllObjects().Num(), 1);
			TestTrue(TEXT("Reconstructed view uses current data"), ContainsObjectiveText(Widget, TEXT("Latest owner snapshot")));
			auto* List = CastChecked<UPanelWidget>(Widget->GetWidgetFromName(TEXT("ObjectiveList")));
			TestEqual(TEXT("Reconstruction creates no duplicate rows"), List->GetChildrenCount(), 1);
		});
	});
}

#endif
