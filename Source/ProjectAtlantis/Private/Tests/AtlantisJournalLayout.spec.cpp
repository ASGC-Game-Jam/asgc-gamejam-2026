// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/PanelSlot.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
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

#endif
