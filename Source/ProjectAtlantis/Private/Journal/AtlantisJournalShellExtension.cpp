// Copyright (c) 2026 ASGC

#include "Journal/AtlantisJournalShellExtension.h"
#include "Journal/AtlantisJournalObjectivesWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "UObject/UnrealType.h"

namespace
{
	constexpr int64 ObjectivesTab = 3;

	// Only supported sibling layouts are extended. Never restructure an unknown authored layout.
	bool AddContentSibling(UWidget* Existing, UUserWidget* Added)
	{
		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(Existing->GetParent()))
		{
			const UCanvasPanelSlot* ExistingSlot = Cast<UCanvasPanelSlot>(Existing->Slot);
			UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Added);
			Slot->SetLayout(ExistingSlot->GetLayout());
			Slot->SetAutoSize(ExistingSlot->GetAutoSize());
			Slot->SetZOrder(ExistingSlot->GetZOrder() + 1);
			return true;
		}
		if (UOverlay* Overlay = Cast<UOverlay>(Existing->GetParent()))
		{
			const UOverlaySlot* ExistingSlot = Cast<UOverlaySlot>(Existing->Slot);
			UOverlaySlot* Slot = Overlay->AddChildToOverlay(Added);
			Slot->SetPadding(ExistingSlot->GetPadding());
			Slot->SetHorizontalAlignment(ExistingSlot->GetHorizontalAlignment());
			Slot->SetVerticalAlignment(ExistingSlot->GetVerticalAlignment());
			return true;
		}
		return false;
	}
}

void UAtlantisJournalShellExtension::Construct()
{
	UUserWidget* Journal = GetUserWidget();
	if (ObjectivesButton)
	{
		return;
	}
	UButton* Investigation = Cast<UButton>(Journal->GetWidgetFromName(TEXT("InvestigationTabButton")));
	UButton* Puzzle = Cast<UButton>(Journal->GetWidgetFromName(TEXT("PuzzleTabButton")));
	UButton* Dialogue = Cast<UButton>(Journal->GetWidgetFromName(TEXT("DialogueTabButton")));
	UWidget* Content = Journal->GetWidgetFromName(TEXT("WBP_JournalInvestigationTab"));
	UHorizontalBox* TabRow = Investigation ? Cast<UHorizontalBox>(Investigation->GetParent()) : nullptr;
	if (!TabRow || !Puzzle || !Dialogue || !Content || GetCurrentTab() == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("Journal Objectives extension could not find the supported existing shell layout."));
		return;
	}
	ObjectivesView = CreateWidget<UAtlantisJournalObjectivesWidget>(Journal->GetGameInstance());
	ObjectivesView->SetVisibility(ESlateVisibility::Collapsed);
	if (!AddContentSibling(Content, ObjectivesView))
	{
		ObjectivesView = nullptr;
		UE_LOG(LogTemp, Warning, TEXT("Journal Objectives content requires a CanvasPanel or Overlay parent; found %s."),
			Content->GetParent() ? *Content->GetParent()->GetClass()->GetName() : TEXT("None"));
		return;
	}
	LegacyContent = Content;
	ObjectivesButton = Journal->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ObjectivesTabButton"));
	UTextBlock* Label = Journal->WidgetTree->ConstructWidget<UTextBlock>();
	Label->SetText(NSLOCTEXT("AtlantisJournal", "ObjectivesTab", "Objectives"));
	ObjectivesButton->AddChild(Label);
	TabRow->AddChildToHorizontalBox(ObjectivesButton)->SetPadding(FMargin(4.f, 0.f));
	ObjectivesButton->OnClicked.AddUniqueDynamic(this, &ThisClass::OnObjectivesClicked);
	LegacyButtons = {Investigation, Puzzle, Dialogue};
	for (UButton* Button : LegacyButtons)
	{
		Button->OnClicked.AddUniqueDynamic(this, &ThisClass::OnLegacyTabClicked);
	}
	// Optional companion #142 view; this worktree does not require its code to compile.
	if (UClass* KnowledgeClass = FindObject<UClass>(nullptr, TEXT("/Script/ProjectAtlantis.AtlantisInvestigationJournalWidget")))
	{
		KnowledgeView = CreateWidget<UUserWidget>(Journal->GetGameInstance(), KnowledgeClass);
		KnowledgeView->SetVisibility(ESlateVisibility::Collapsed);
		if (!AddContentSibling(Content, KnowledgeView))
		{
			KnowledgeView = nullptr;
		}
	}
	RefreshKnowledgeView();
}

void UAtlantisJournalShellExtension::Destruct()
{
	ShowLegacyContent();
	for (UButton* Button : LegacyButtons)
	{
		if (Button)
		{
			Button->OnClicked.RemoveDynamic(this, &ThisClass::OnLegacyTabClicked);
		}
	}
	LegacyButtons.Reset();
	if (ObjectivesButton)
	{
		ObjectivesButton->OnClicked.RemoveDynamic(this, &ThisClass::OnObjectivesClicked);
		ObjectivesButton->RemoveFromParent();
		ObjectivesButton = nullptr;
	}
	if (ObjectivesView)
	{
		ObjectivesView->RemoveFromParent();
		ObjectivesView = nullptr;
	}
	if (KnowledgeView)
	{
		KnowledgeView->RemoveFromParent();
		KnowledgeView = nullptr;
	}
	LegacyContent = nullptr;
	LastKnowledgeTab = INDEX_NONE;
}

int64 UAtlantisJournalShellExtension::GetCurrentTab() const
{
	FProperty* Property = GetUserWidget()->GetClass()->FindPropertyByName(TEXT("CurrentTabType"));
	if (FEnumProperty* Enum = CastField<FEnumProperty>(Property))
	{
		return Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(GetUserWidget()));
	}
	if (FByteProperty* Byte = CastField<FByteProperty>(Property))
	{
		return Byte->GetPropertyValue_InContainer(GetUserWidget());
	}
	return INDEX_NONE;
}

void UAtlantisJournalShellExtension::SetCurrentTab(int64 Tab)
{
	FProperty* Property = GetUserWidget()->GetClass()->FindPropertyByName(TEXT("CurrentTabType"));
	if (FEnumProperty* Enum = CastField<FEnumProperty>(Property))
	{
		Enum->GetUnderlyingProperty()->SetIntPropertyValue(Enum->ContainerPtrToValuePtr<void>(GetUserWidget()), Tab);
	}
	else if (FByteProperty* Byte = CastField<FByteProperty>(Property))
	{
		Byte->SetPropertyValue_InContainer(GetUserWidget(), static_cast<uint8>(Tab));
	}
}

void UAtlantisJournalShellExtension::OnObjectivesClicked()
{
	if (!ObjectivesView || !LegacyContent)
	{
		return;
	}
	if (!bShowingObjectives)
	{
		PreviousTab = GetCurrentTab();
		PreviousContentVisibility = LegacyContent->GetVisibility();
	}
	bShowingObjectives = true;
	SetCurrentTab(ObjectivesTab);
	LegacyContent->SetVisibility(ESlateVisibility::Collapsed);
	ObjectivesView->SetVisibility(ESlateVisibility::Visible);
	RefreshKnowledgeView();
}

void UAtlantisJournalShellExtension::ShowLegacyContent()
{
	if (bShowingObjectives)
	{
		if (GetCurrentTab() == ObjectivesTab)
		{
			SetCurrentTab(PreviousTab);
			if (LegacyContent)
			{
				LegacyContent->SetVisibility(PreviousContentVisibility);
			}
		}
		bShowingObjectives = false;
	}
	if (ObjectivesView)
	{
		ObjectivesView->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAtlantisJournalShellExtension::OnLegacyTabClicked()
{
	ShowLegacyContent();
	RefreshKnowledgeView();
}

void UAtlantisJournalShellExtension::RefreshKnowledgeView()
{
	if (!KnowledgeView)
	{
		return;
	}
	const int64 Tab = GetCurrentTab();
	const bool bShowKnowledge = !bShowingObjectives && (Tab == 0 || Tab == 1);
	KnowledgeView->SetVisibility(bShowKnowledge ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (bShowKnowledge)
	{
		if (Tab != LastKnowledgeTab)
		{
			if (UFunction* Filter = KnowledgeView->FindFunction(TEXT("SetKnowledgeFilter")))
			{
				struct FFilterParameters { bool bShowPuzzles; } Parameters{Tab == 1};
				KnowledgeView->ProcessEvent(Filter, &Parameters);
			}
			LastKnowledgeTab = Tab;
		}
		LegacyContent->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UAtlantisJournalShellExtension::Tick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (bShowingObjectives && GetCurrentTab() != ObjectivesTab)
	{
		ShowLegacyContent();
	}
	RefreshKnowledgeView();
}

void UAtlantisJournalShellExtensionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	DiscoveryTicker = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ThisClass::OnDiscoverJournal), 0.25f);
}

void UAtlantisJournalShellExtensionSubsystem::Deinitialize()
{
	FTSTicker::GetCoreTicker().RemoveTicker(DiscoveryTicker);
	DiscoveryTicker.Reset();
	Super::Deinitialize();
}

bool UAtlantisJournalShellExtensionSubsystem::OnDiscoverJournal(float DeltaTime)
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return true;
	}
	UClass* JournalClass = FindObject<UClass>(nullptr, TEXT("/Game/ProjectAtlantis/UI/WBP_Journal.WBP_Journal_C"));
	if (!JournalClass)
	{
		return true;
	}
	TArray<UUserWidget*> Journals;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetGameInstance(), Journals, JournalClass, false);
	for (UUserWidget* Journal : Journals)
	{
		if (Journal->GetClass() == JournalClass && Journal->GetWorld() == World && Journal->IsInViewport()
			&& !Journal->GetExtension<UAtlantisJournalShellExtension>())
		{
			Journal->AddExtension<UAtlantisJournalShellExtension>();
		}
	}
	return true;
}
