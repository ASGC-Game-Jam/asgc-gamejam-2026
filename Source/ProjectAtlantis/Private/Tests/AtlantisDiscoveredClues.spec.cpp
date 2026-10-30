// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Journal/AtlantisJournalSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "CommonGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	FJournalInvestigationEntry Placeholder(FName Id = TEXT("Story435_Placeholder"))
	{
		FJournalInvestigationEntry Entry;
		Entry.EntryId = Id;
		Entry.Title = FText::FromString(TEXT("[Placeholder] Discovered clue"));
		Entry.Descriptions = { FText::FromString(TEXT("Placeholder clue content for journal verification.")) };
		Entry.Status = EJournalInvestigationStatus::Inprogress;
		return Entry;
	}

	FJournalInvestigationEntry GroupedPlaceholder(FName Id, FName District, FName Mystery)
	{
		FJournalInvestigationEntry Entry = Placeholder(Id);
		Entry.DistrictId = District;
		Entry.DistrictTitle = FText::FromString(TEXT("[Placeholder] Equal district heading"));
		Entry.MysteryId = Mystery;
		Entry.MysteryTitle = FText::FromString(TEXT("[Placeholder] Equal mystery heading"));
		return Entry;
	}

	UPanelWidget* GroupContents(UWidget* Widget)
	{
		UUserWidget* Group = Cast<UUserWidget>(Widget);
		return Group ? Cast<UPanelWidget>(Group->GetWidgetFromName(TEXT("GroupContents"))) : nullptr;
	}

	FName ViewGroupId(UWidget* Widget)
	{
		const FNameProperty* Property = Widget ? FindFProperty<FNameProperty>(Widget->GetClass(), TEXT("GroupId")) : nullptr;
		return Property ? Property->GetPropertyValue_InContainer(Widget) : NAME_None;
	}

	struct FDiscoveredClueFixture
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		UWorld* World = nullptr;
		ULocalPlayer* Player = nullptr;
		UAtlantisJournalSubsystem* Journal = nullptr;
		UUserWidget* Shell = nullptr;
		UUserWidget* Tab = nullptr;
		TSharedPtr<SWidget> Slate;

		FDiscoveredClueFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine))
		{
			GameInstance->InitializeStandalone();
			World = GameInstance->GetWorld();
			Journal = GameInstance->GetSubsystem<UAtlantisJournalSubsystem>();
			UCommonGameViewportClient* Viewport = NewObject<UCommonGameViewportClient>(GEngine);
			GameInstance->GetWorldContext()->GameViewport = Viewport;
			Viewport->Init(*GameInstance->GetWorldContext(), GameInstance.Get(), false);
			FString Error;
			Player = GameInstance->CreateLocalPlayer(0, Error, false);
			APlayerController* Controller = World->SpawnActor<APlayerController>();
			Controller->SetPlayer(Player);
			World->AddController(Controller);
			UClass* Class = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ProjectAtlantis/UI/WBP_Journal.WBP_Journal_C"));
			Shell = Class ? CreateWidget<UUserWidget>(Controller, Class) : nullptr;
			if (Shell)
			{
				Slate = Shell->TakeWidget();
				Tab = Cast<UUserWidget>(Shell->GetWidgetFromName(TEXT("WBP_JournalInvestigationTab")));
			}
		}

		~FDiscoveredClueFixture()
		{
			if (Shell)
			{
				Shell->RemoveFromParent();
				Shell->ReleaseSlateResources(true);
			}
			Slate.Reset();
			if (Player) { GameInstance->RemoveLocalPlayer(Player); }
			GameInstance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}

		UPanelWidget* List() const { return Tab ? Cast<UPanelWidget>(Tab->GetWidgetFromName(TEXT("ClueList"))) : nullptr; }
		UTextBlock* Empty() const { return Tab ? Cast<UTextBlock>(Tab->GetWidgetFromName(TEXT("EmptyState"))) : nullptr; }
	};
}

BEGIN_DEFINE_SPEC(FAtlantisDiscoveredCluesSpec, "Atlantis.Journal.DiscoveredClues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FDiscoveredClueFixture> Fixture;
END_DEFINE_SPEC(FAtlantisDiscoveredCluesSpec)

void FAtlantisDiscoveredCluesSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FDiscoveredClueFixture>();
		TestNotNull(TEXT("Existing journal subsystem"), Fixture->Journal);
		TestNotNull(TEXT("Actual journal shell"), Fixture->Shell);
		TestNotNull(TEXT("Blueprint investigation tab"), Fixture->Tab);
		TestNotNull(TEXT("Blueprint clue list"), Fixture->List());
		TestNotNull(TEXT("Blueprint empty state"), Fixture->Empty());
	});
	AfterEach([this]() { Fixture.Reset(); });

	Describe(TEXT("Existing entry store"), [this]()
	{
		It(TEXT("should preserve identity separately from equal display titles"), [this]()
		{
			if (!Fixture->List()) { return; }
			Fixture->Journal->SetInvestigationEntry(Placeholder(TEXT("First")));
			Fixture->Journal->SetInvestigationEntry(Placeholder(TEXT("Second")));
			TestEqual(TEXT("Two independent entries"), Fixture->Journal->GetInvestigationEntries().Num(), 2);
			TestEqual(TEXT("Two visible rows"), Fixture->List()->GetChildrenCount(), 2);
			Fixture->Journal->SetInvestigationEntry(Placeholder(TEXT("FIRST")));
			TestEqual(TEXT("Equivalent FName identity does not add another entry"), Fixture->Journal->GetInvestigationEntries().Num(), 2);
			TestEqual(TEXT("Equivalent identity does not add another row"), Fixture->List()->GetChildrenCount(), 2);
		});
		It(TEXT("should keep legacy description-only entries undiscovered until their owner discloses them"), [this]()
		{
			if (!Fixture->List()) { return; }
			Fixture->Journal->UpdateInvestigationDescription(TEXT("Legacy placeholder"), TEXT("Legacy placeholder description"));
			TestEqual(TEXT("Default status"), Fixture->Journal->GetInvestigationStatus(TEXT("Legacy placeholder")), EJournalInvestigationStatus::Undiscovered);
			TestEqual(TEXT("Description does not disclose"), Fixture->List()->GetChildrenCount(), 0);
			Fixture->Journal->UpdateInvestigationStatus(TEXT("Legacy placeholder"), EJournalInvestigationStatus::Inprogress);
			TestEqual(TEXT("Legacy content uses same view"), Fixture->List()->GetChildrenCount(), 1);
			TestEqual(TEXT("Legacy getter preserved"), Fixture->Journal->GetInvestigationDescriptions(TEXT("Legacy placeholder"))[0], FString(TEXT("Legacy placeholder description")));
		});
		It(TEXT("should reject invalid identity or title without changing stored content"), [this]()
		{
			if (!Fixture->List()) { return; }
			Fixture->Journal->SetInvestigationEntry(Placeholder());
			FJournalInvestigationEntry Invalid = Placeholder(NAME_None);
			TestFalse(TEXT("Missing identity rejected"), Fixture->Journal->SetInvestigationEntry(Invalid));
			Invalid = Placeholder();
			Invalid.Title = FText::FromString(TEXT("  "));
			TestFalse(TEXT("Blank title rejected"), Fixture->Journal->SetInvestigationEntry(Invalid));
			TestEqual(TEXT("Original entry retained"), Fixture->Journal->GetInvestigationEntries().Num(), 1);
		});
	});

	Describe(TEXT("Widget Blueprint view"), [this]()
	{
		It(TEXT("should show only disclosed content once and restore the empty state on clear"), [this]()
		{
			if (!Fixture->List() || !Fixture->Empty()) { return; }
			TestTrue(TEXT("Initially empty"), Fixture->Empty()->IsVisible());
			FJournalInvestigationEntry Hidden = Placeholder(TEXT("Hidden"));
			Hidden.Status = EJournalInvestigationStatus::Undiscovered;
			Fixture->Journal->SetInvestigationEntry(Hidden);
			TestEqual(TEXT("Undiscovered hidden"), Fixture->List()->GetChildrenCount(), 0);
			Fixture->Journal->SetInvestigationEntry(Placeholder());
			Fixture->Journal->SetInvestigationEntry(Placeholder());
			TestEqual(TEXT("Repeated delivery gives one visible row"), Fixture->List()->GetChildrenCount(), 1);
			TestEqual(TEXT("Model retains both owner-supplied entries"), Fixture->Journal->GetInvestigationEntries().Num(), 2);
			TestFalse(TEXT("Empty message collapsed"), Fixture->Empty()->IsVisible());
			UUserWidget* Row = Cast<UUserWidget>(Fixture->List()->GetChildAt(0));
			if (!TestNotNull(TEXT("Blueprint row"), Row)) { return; }
			UTextBlock* Title = Cast<UTextBlock>(Row->GetWidgetFromName(TEXT("ClueTitle")));
			if (TestNotNull(TEXT("Authored title widget"), Title))
			{
				TestEqual(TEXT("Public title rendered"), Title->GetText().ToString(), Placeholder().Title.ToString());
			}
			UPanelWidget* Descriptions = Cast<UPanelWidget>(Row->GetWidgetFromName(TEXT("ClueDescriptions")));
			if (TestNotNull(TEXT("Authored descriptions panel"), Descriptions))
			{
				TestEqual(TEXT("Description rendered once"), Descriptions->GetChildrenCount(), 1);
				UUserWidget* DescriptionRow = Cast<UUserWidget>(Descriptions->GetChildAt(0));
				UTextBlock* Description = DescriptionRow ? Cast<UTextBlock>(DescriptionRow->GetWidgetFromName(TEXT("Description"))) : nullptr;
				if (TestNotNull(TEXT("Existing description widget reused"), Description))
				{
					TestEqual(TEXT("Public description rendered"), Description->GetText().ToString(), Placeholder().Descriptions[0].ToString());
				}
			}
			Fixture->Journal->ClearInvestigations();
			TestEqual(TEXT("Rows cleared"), Fixture->List()->GetChildrenCount(), 0);
			TestTrue(TEXT("Empty message restored"), Fixture->Empty()->IsVisible());
		});
		It(TEXT("should unbind on destruct and bind once when reconstructed"), [this]()
		{
			if (!Fixture->Tab || !Fixture->List()) { return; }
			const auto CountBindings = [this]()
			{
				return Fixture->Journal->OnInvestigationUpdated.GetAllObjects().FilterByPredicate([this](UObject* Object) { return Object == Fixture->Tab; }).Num();
			};
			TestEqual(TEXT("One entry subscription"), CountBindings(), 1);
			Fixture->Tab->Destruct();
			TestEqual(TEXT("Entry subscription removed"), CountBindings(), 0);
			TestFalse(TEXT("Clear subscription removed"), Fixture->Journal->OnJournalInvestigationsCleared.Contains(Fixture->Tab, TEXT("OnJournalEntriesCleared")));
			Fixture->Journal->SetInvestigationEntry(Placeholder());
			TestEqual(TEXT("Detached view not refreshed"), Fixture->List()->GetChildrenCount(), 0);
			Fixture->Tab->Construct();
			Fixture->Tab->Construct();
			TestEqual(TEXT("Reconstruction keeps one subscription"), CountBindings(), 1);
			TestEqual(TEXT("Current content loaded on reconstruction"), Fixture->List()->GetChildrenCount(), 1);
		});
	});

	Describe(TEXT("District and mystery grouping"), [this]()
	{
		It(TEXT("should group by district and mystery identities even when headings match"), [this]()
		{
			UPanelWidget* List = Fixture->List();
			if (!List) { return; }
			for (const FJournalInvestigationEntry& Entry : {
				GroupedPlaceholder(TEXT("D"), TEXT("DistrictB"), TEXT("MysteryOne")),
				GroupedPlaceholder(TEXT("C"), TEXT("DistrictA"), TEXT("MysteryTwo")),
				GroupedPlaceholder(TEXT("B"), TEXT("DistrictA"), TEXT("MysteryOne")),
				GroupedPlaceholder(TEXT("A"), TEXT("DistrictA"), TEXT("MysteryOne")) })
			{
				Fixture->Journal->SetInvestigationEntry(Entry);
			}
			if (!TestEqual(TEXT("Equal district titles retain two districts"), List->GetChildrenCount(), 2)) { return; }
			TestEqual(TEXT("First district identity"), ViewGroupId(List->GetChildAt(0)), FName(TEXT("DistrictA")));
			TestEqual(TEXT("Second district identity"), ViewGroupId(List->GetChildAt(1)), FName(TEXT("DistrictB")));
			UPanelWidget* FirstDistrict = GroupContents(List->GetChildAt(0));
			UPanelWidget* SecondDistrict = GroupContents(List->GetChildAt(1));
			if (!TestNotNull(TEXT("First district content"), FirstDistrict) || !TestNotNull(TEXT("Second district content"), SecondDistrict)) { return; }
			if (!TestEqual(TEXT("Equal mystery titles retain two mysteries"), FirstDistrict->GetChildrenCount(), 2)) { return; }
			TestEqual(TEXT("Other district has its own mystery"), SecondDistrict->GetChildrenCount(), 1);
			TestEqual(TEXT("First mystery identity"), ViewGroupId(FirstDistrict->GetChildAt(0)), FName(TEXT("MysteryOne")));
			TestEqual(TEXT("Second mystery identity"), ViewGroupId(FirstDistrict->GetChildAt(1)), FName(TEXT("MysteryTwo")));
			UPanelWidget* FirstMystery = GroupContents(FirstDistrict->GetChildAt(0));
			UPanelWidget* SecondMystery = GroupContents(FirstDistrict->GetChildAt(1));
			UPanelWidget* OtherMystery = GroupContents(SecondDistrict->GetChildAt(0));
			if (!TestNotNull(TEXT("First mystery content"), FirstMystery) || !TestNotNull(TEXT("Second mystery content"), SecondMystery) || !TestNotNull(TEXT("Other district mystery content"), OtherMystery)) { return; }
			TestEqual(TEXT("Related clues share one mystery"), FirstMystery->GetChildrenCount(), 2);
			TestEqual(TEXT("Separate mystery has its own clue"), SecondMystery->GetChildrenCount(), 1);
			TestEqual(TEXT("Same mystery ID in another district stays separate"), OtherMystery->GetChildrenCount(), 1);
		});

		It(TEXT("should expose no headings for undiscovered groups and clear visible groups"), [this]()
		{
			if (!Fixture->List() || !Fixture->Empty()) { return; }
			FJournalInvestigationEntry Hidden = GroupedPlaceholder(TEXT("Hidden"), TEXT("HiddenDistrict"), TEXT("HiddenMystery"));
			Hidden.Status = EJournalInvestigationStatus::Undiscovered;
			Fixture->Journal->SetInvestigationEntry(Hidden);
			TestEqual(TEXT("Hidden groups create no headings"), Fixture->List()->GetChildrenCount(), 0);
			TestTrue(TEXT("Empty state remains visible"), Fixture->Empty()->IsVisible());
			Fixture->Journal->SetInvestigationEntry(GroupedPlaceholder(TEXT("Visible"), TEXT("VisibleDistrict"), TEXT("VisibleMystery")));
			TestEqual(TEXT("Only disclosed district appears"), Fixture->List()->GetChildrenCount(), 1);
			TestEqual(TEXT("Disclosed district identity"), ViewGroupId(Fixture->List()->GetChildAt(0)), FName(TEXT("VisibleDistrict")));
			Fixture->Journal->ClearInvestigations();
			TestEqual(TEXT("All group headings cleared"), Fixture->List()->GetChildrenCount(), 0);
			TestTrue(TEXT("Empty state restored"), Fixture->Empty()->IsVisible());
		});

		It(TEXT("should keep group ordering deterministic across delivery orders and refreshes"), [this]()
		{
			if (!Fixture->List()) { return; }
			const FJournalInvestigationEntry First = GroupedPlaceholder(TEXT("A"), TEXT("DistrictZ"), TEXT("MysteryZ"));
			const FJournalInvestigationEntry Second = GroupedPlaceholder(TEXT("B"), TEXT("DistrictA"), TEXT("MysteryA"));
			Fixture->Journal->SetInvestigationEntry(Second);
			Fixture->Journal->SetInvestigationEntry(First);
			if (!TestEqual(TEXT("Two ordered districts"), Fixture->List()->GetChildrenCount(), 2)) { return; }
			const FName BeforeFirst = ViewGroupId(Fixture->List()->GetChildAt(0));
			const FName BeforeSecond = ViewGroupId(Fixture->List()->GetChildAt(1));
			Fixture->Journal->ClearInvestigations();
			Fixture->Journal->SetInvestigationEntry(First);
			Fixture->Journal->SetInvestigationEntry(Second);
			Fixture->Journal->SetInvestigationEntry(First);
			if (!TestEqual(TEXT("Refresh retains two districts"), Fixture->List()->GetChildrenCount(), 2)) { return; }
			TestEqual(TEXT("First group independent of delivery order"), ViewGroupId(Fixture->List()->GetChildAt(0)), BeforeFirst);
			TestEqual(TEXT("Second group independent of delivery order"), ViewGroupId(Fixture->List()->GetChildAt(1)), BeforeSecond);
		});
	});

}
#endif
