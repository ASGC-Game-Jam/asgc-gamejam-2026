// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Core/AtlantisPlayerController.h"
#include "Journal/AtlantisJournalShellExtension.h"
#include "Journal/AtlantisJournalObjectivesWidget.h"
#include "Journal/AtlantisJournalObjectivesSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Components/TextBlock.h"
#include "CommonGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisJournalLayoutTest,
	"Atlantis.Journal.ExistingShellLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisJournalLayoutTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GEngine));
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	UClass* JournalClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/ProjectAtlantis/UI/WBP_Journal.WBP_Journal_C"));
	UUserWidget* Journal = JournalClass ? CreateWidget<UUserWidget>(GameInstance.Get(), JournalClass) : nullptr;
	if (TestNotNull(TEXT("Existing Journal asset loads"), Journal))
	{
		Journal->WidgetTree->ForEachWidget([this](UWidget* Widget)
		{
			AddInfo(FString::Printf(TEXT("JournalLayout Name=%s Class=%s Parent=%s ParentClass=%s Slot=%s"),
				*Widget->GetName(), *Widget->GetClass()->GetName(),
				Widget->GetParent() ? *Widget->GetParent()->GetName() : TEXT("None"),
				Widget->GetParent() ? *Widget->GetParent()->GetClass()->GetName() : TEXT("None"),
				Widget->Slot ? *Widget->Slot->GetClass()->GetName() : TEXT("None")));
		});
		for (const TCHAR* Name : {TEXT("InvestigationTabButton"), TEXT("PuzzleTabButton"), TEXT("DialogueTabButton")})
		{
			TestNotNull(Name, Cast<UButton>(Journal->GetWidgetFromName(Name)));
		}
		TestNotNull(TEXT("Existing investigation content"), Journal->GetWidgetFromName(TEXT("WBP_JournalInvestigationTab")));
		for (TFieldIterator<FProperty> It(JournalClass); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Tab")))
			{
				FString Value;
				It->ExportText_InContainer(0, Value, Journal, Journal, Journal, PPF_None);
				AddInfo(FString::Printf(TEXT("JournalProperty %s=%s"), *It->GetName(), *Value));
			}
		}
	}
	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisJournalShellExtensionTest,
	"Atlantis.Journal.ExistingShellExtension",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisJournalShellExtensionTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>(GEngine));
	GameInstance->InitializeStandalone();
	UWorld* World = GameInstance->GetWorld();
	UCommonGameViewportClient* Viewport = NewObject<UCommonGameViewportClient>(GEngine);
	GameInstance->GetWorldContext()->GameViewport = Viewport;
	Viewport->Init(*GameInstance->GetWorldContext(), GameInstance.Get(), false);
	FString PlayerError;
	ULocalPlayer* LocalPlayer = GameInstance->CreateLocalPlayer(0, PlayerError, false);
	AAtlantisPlayerController* Controller = World->SpawnActor<AAtlantisPlayerController>();
	Controller->SetPlayer(LocalPlayer);
	World->AddController(Controller);
	UClass* JournalClass = LoadClass<UUserWidget>(nullptr,
		TEXT("/Game/ProjectAtlantis/UI/WBP_Journal.WBP_Journal_C"));
	UUserWidget* Journal = JournalClass ? CreateWidget<UUserWidget>(Controller, JournalClass) : nullptr;
	if (TestNotNull(TEXT("Existing Journal created with its real player controller"), Journal))
	{
		TSharedRef<SWidget> SlateWidget = Journal->TakeWidget();
		UWidget* LegacyContent = Journal->GetWidgetFromName(TEXT("WBP_JournalInvestigationTab"));
		UPanelWidget* OriginalParent = LegacyContent->GetParent();
		UButton* Investigation = Cast<UButton>(Journal->GetWidgetFromName(TEXT("InvestigationTabButton")));
		UButton* Puzzle = Cast<UButton>(Journal->GetWidgetFromName(TEXT("PuzzleTabButton")));
		UButton* Dialogue = Cast<UButton>(Journal->GetWidgetFromName(TEXT("DialogueTabButton")));
		const auto CurrentTab = [Journal]() -> uint8
		{
			FByteProperty* Property = FindFProperty<FByteProperty>(Journal->GetClass(), TEXT("CurrentTabType"));
			return Property ? Property->GetPropertyValue_InContainer(Journal) : 255;
		};
		UAtlantisJournalShellExtension* Extension = Journal->AddExtension<UAtlantisJournalShellExtension>();
		UButton* Objectives = Cast<UButton>(Journal->GetWidgetFromName(TEXT("ObjectivesTabButton")));
		if (TestNotNull(TEXT("Objectives button added to existing shell"), Objectives))
		{
			TestEqual(TEXT("Existing content hierarchy preserved"), LegacyContent->GetParent(), OriginalParent);
			TestEqual(TEXT("Objectives joins existing tab row"), Objectives->GetParent(), Investigation->GetParent());
			UAtlantisJournalObjectivesWidget* View = nullptr;
			for (UWidget* Child : OriginalParent->GetAllChildren())
			{
				if (UAtlantisJournalObjectivesWidget* ObjectiveView = Cast<UAtlantisJournalObjectivesWidget>(Child))
				{
					View = ObjectiveView;
				}
			}
			if (TestNotNull(TEXT("Native view inserted beside legacy content"), View))
			{
				TestTrue(TEXT("Objectives starts hidden"), View->GetVisibility() == ESlateVisibility::Collapsed);
				Objectives->OnClicked.Broadcast();
				TestEqual(TEXT("Objectives selection updates shell tab state"), CurrentTab(), static_cast<uint8>(3));
				TestTrue(TEXT("Objectives click displays view"), View->GetVisibility() == ESlateVisibility::Visible);
				TestTrue(TEXT("Objectives hides legacy content"), LegacyContent->GetVisibility() == ESlateVisibility::Collapsed);
				FJournalObjectivePresentation Objective;
				Objective.ObjectiveId = TEXT("ShellTest");
				Objective.Text = FText::FromString(TEXT("Owner update inside the existing shell"));
				GameInstance->GetSubsystem<UAtlantisJournalObjectivesSubsystem>()->SubmitObjectiveSnapshot({Objective});
				bool bUpdated = false;
				View->WidgetTree->ForEachWidget([&bUpdated](UWidget* Child)
				{
					if (UTextBlock* Label = Cast<UTextBlock>(Child))
					{
						bUpdated |= Label->GetText().ToString().Contains(TEXT("Owner update inside the existing shell"));
					}
				});
				TestTrue(TEXT("Embedded objective text refreshes"), bUpdated);
				Puzzle->OnClicked.Broadcast();
				TestEqual(TEXT("Existing Puzzle button retains Blueprint navigation"), CurrentTab(), static_cast<uint8>(1));
				TestTrue(TEXT("Existing tab exits Objectives"), View->GetVisibility() == ESlateVisibility::Collapsed);
				Objectives->OnClicked.Broadcast();
				Dialogue->OnClicked.Broadcast();
				TestEqual(TEXT("Existing Dialogue button retains navigation"), CurrentTab(), static_cast<uint8>(2));
				Investigation->OnClicked.Broadcast();
				TestEqual(TEXT("Existing Investigation button retains navigation"), CurrentTab(), static_cast<uint8>(0));
				Objectives->OnClicked.Broadcast();
				FindFProperty<FByteProperty>(Journal->GetClass(), TEXT("CurrentTabType"))->SetPropertyValue_InContainer(Journal, 1);
				Extension->Tick(FGeometry(), 0.f);
				TestTrue(TEXT("Blueprint tab state updates also exit Objectives"), View->GetVisibility() == ESlateVisibility::Collapsed);
			}
			Extension->Destruct();
			const uint8 TabAfterDestruct = CurrentTab();
			Objectives->OnClicked.Broadcast();
			TestEqual(TEXT("Detached button no longer changes shell state"), CurrentTab(), TabAfterDestruct);
			TestNull(TEXT("Destruct removes inserted button"), Journal->GetWidgetFromName(TEXT("ObjectivesTabButton")));
			Extension->Construct();
			TestNotNull(TEXT("Extension reconstructs when shell reopens"), Journal->GetWidgetFromName(TEXT("ObjectivesTabButton")));
			Extension->Construct();
			int32 ObjectiveButtons = 0;
			Journal->WidgetTree->ForEachWidget([&ObjectiveButtons](UWidget* Child)
			{
				ObjectiveButtons += Child->GetFName() == TEXT("ObjectivesTabButton") ? 1 : 0;
			});
			TestEqual(TEXT("Repeated construction does not duplicate button"), ObjectiveButtons, 1);
		}
		Journal->RemoveExtension(Extension);
		Journal->ReleaseSlateResources(true);
	}
	GameInstance->RemoveLocalPlayer(LocalPlayer);
	GameInstance->Shutdown();
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
