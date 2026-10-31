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
				Tab = Cast<UUserWidget>(Shell->GetWidgetFromName(TEXT("InvestigationTab")));
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
}
#endif
