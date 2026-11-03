// Copyright (c) 2026 ASGC
#include "Journal/AtlantisJournalEmptyStateExtension.h"
#include "Journal/AtlantisJournalShellExtension.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "UObject/UnrealType.h"

void UAtlantisJournalEmptyStateExtension::Construct()
{
	UUserWidget* Shell = GetUserWidget();
	if (EmptyState || !Shell) { return; }
	UWidget* Content = Shell->GetWidgetFromName(TEXT("InvestigationTab"));
	UVerticalBox* Column = Content ? Cast<UVerticalBox>(Content->GetParent()) : nullptr;
	const TCHAR* Names[] = { TEXT("InvestigationTabButton"), TEXT("PuzzleTabButton"), TEXT("DialogueTabButton") };
	for (const TCHAR* Name : Names)
	{
		UButton* Button = Cast<UButton>(Shell->GetWidgetFromName(Name));
		if (!Button) { TabButtons.Reset(); return; }
		TabButtons.Add(Button);
	}
	if (!Column || !FindFProperty<FByteProperty>(Shell->GetClass(), TEXT("CurrentTabType")))
	{
		TabButtons.Reset(); return;
	}
	EmptyState = Shell->WidgetTree->ConstructWidget<UTextBlock>();
	EmptyState->SetAutoWrapText(true);
	Column->AddChildToVerticalBox(EmptyState)->SetPadding(FMargin(16.f));
	for (UButton* Button : TabButtons) { Button->OnClicked.AddUniqueDynamic(this, &ThisClass::RefreshEmptyState); }
	if (UGameInstance* GameInstance = Shell->GetGameInstance())
	{
		JournalData = GameInstance->GetSubsystem<UAtlantisJournalSubsystem>();
		if (JournalData.IsValid())
		{
			JournalData->OnInvestigationUpdated.AddUniqueDynamic(this, &ThisClass::OnInvestigationUpdated);
			JournalData->OnJournalInvestigationsCleared.AddUniqueDynamic(this, &ThisClass::RefreshEmptyState);
		}
	}
	RefreshEmptyState();
}

void UAtlantisJournalEmptyStateExtension::Destruct()
{
	for (UButton* Button : TabButtons)
	{
		if (Button) { Button->OnClicked.RemoveDynamic(this, &ThisClass::RefreshEmptyState); }
	}
	TabButtons.Reset();
	if (JournalData.IsValid())
	{
		JournalData->OnInvestigationUpdated.RemoveDynamic(this, &ThisClass::OnInvestigationUpdated);
		JournalData->OnJournalInvestigationsCleared.RemoveDynamic(this, &ThisClass::RefreshEmptyState);
	}
	JournalData.Reset();
	if (EmptyState) { EmptyState->RemoveFromParent(); EmptyState = nullptr; }
}

void UAtlantisJournalEmptyStateExtension::OnInvestigationUpdated(EJournalInvestigationStatus OldStatus,
	EJournalInvestigationStatus Status, const FString& Title)
{
	RefreshEmptyState();
}

void UAtlantisJournalEmptyStateExtension::RefreshEmptyState()
{
	if (!EmptyState) { return; }
	if (const UAtlantisJournalShellExtension* NativeSections = GetUserWidget()->GetExtension<UAtlantisJournalShellExtension>();
		NativeSections && NativeSections->IsShowingNativeContent())
	{
		// Knowledge and Objectives already render their own filtered empty-state content.
		EmptyState->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	const FByteProperty* Tab = FindFProperty<FByteProperty>(GetUserWidget()->GetClass(), TEXT("CurrentTabType"));
	const uint8 Selection = Tab->GetPropertyValue_InContainer(GetUserWidget());
	// These are the existing authored EJournalTabType values, not a second navigation model.
	bool bEmpty = true;
	if (Selection == 0)
	{
		if (JournalData.IsValid())
		{
			const TArray<FJournalInvestigationEntry> Entries = JournalData->GetInvestigationEntries();
			bEmpty = !Entries.ContainsByPredicate([](const FJournalInvestigationEntry& Entry)
			{
				return Entry.Status != EJournalInvestigationStatus::Undiscovered && Entry.Kind == EJournalEntryKind::Clue;
			});
		}
		EmptyState->SetText(NSLOCTEXT("AtlantisJournal", "NoInvestigations", "No investigations discovered yet."));
	}
	else if (Selection == 1)
	{
		if (JournalData.IsValid())
		{
			const TArray<FJournalInvestigationEntry> Entries = JournalData->GetInvestigationEntries();
			bEmpty = !Entries.ContainsByPredicate([](const FJournalInvestigationEntry& Entry)
			{
				return Entry.Status != EJournalInvestigationStatus::Undiscovered && Entry.Kind == EJournalEntryKind::PuzzleObservation;
			});
		}
		EmptyState->SetText(NSLOCTEXT("AtlantisJournal", "NoPuzzles", "No puzzle entries yet."));
	}
	else if (Selection == 2)
	{
		EmptyState->SetText(NSLOCTEXT("AtlantisJournal", "NoDialogue", "No dialogue entries yet."));
	}
	else { bEmpty = false; }
	EmptyState->SetVisibility(bEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
