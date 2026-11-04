// Copyright (c) 2026 ASGC

#include "Journal/AtlantisJournalObjectivesWidget.h"
#include "Journal/AtlantisJournalSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"

#define LOCTEXT_NAMESPACE "AtlantisJournalObjectives"

void UAtlantisJournalObjectivesWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		Journal = GameInstance->GetSubsystem<UAtlantisJournalSubsystem>();
		Journal->OnObjectivesChanged.AddUniqueDynamic(this, &ThisClass::RefreshObjectives);
	}
	RefreshObjectives();
}

void UAtlantisJournalObjectivesWidget::NativeDestruct()
{
	if (Journal)
	{
		Journal->OnObjectivesChanged.RemoveDynamic(this, &ThisClass::RefreshObjectives);
		Journal = nullptr;
	}
	Super::NativeDestruct();
}

void UAtlantisJournalObjectivesWidget::RefreshObjectives()
{
	if (!ObjectiveList || !EmptyState)
	{
		return;
	}
	ObjectiveList->ClearChildren();
	const auto AddText = [this](const FText& Text, const FLinearColor& Color, float BottomPadding)
	{
		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
		Label->SetText(Text);
		Label->SetAutoWrapText(true);
		Label->SetColorAndOpacity(FSlateColor(Color));
		UVerticalBoxSlot* Slot = ObjectiveList->AddChildToVerticalBox(Label);
		Slot->SetPadding(FMargin(12.f, 4.f, 12.f, BottomPadding));
		return Slot;
	};
	const TArray<FJournalObjectivePresentation> Visible = Journal ? Journal->GetVisibleObjectives() : TArray<FJournalObjectivePresentation>();
	EmptyState->SetVisibility(Visible.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (const FJournalObjectivePresentation& Objective : Visible)
	{
		FText State;
		FLinearColor Color = FLinearColor::White;
		switch (Objective.State)
		{
		case EJournalObjectiveDisplayState::Completed:
			State = LOCTEXT("Completed", "Completed");
			Color = FLinearColor(0.55f, 0.9f, 0.65f);
			break;
		case EJournalObjectiveDisplayState::Failed:
			State = LOCTEXT("Failed", "Failed");
			Color = FLinearColor(1.f, 0.6f, 0.55f);
			break;
		default:
			State = LOCTEXT("Active", "Active");
			break;
		}
		UVerticalBoxSlot* LastText = AddText(FText::Format(LOCTEXT("Objective", "{0}: {1}"), State, Objective.Text), Color, 4.f);
		if (!Objective.Context.IsEmptyOrWhitespace())
		{
			LastText = AddText(Objective.Context, FLinearColor::White, 4.f);
		}
		if (!Objective.Location.IsEmptyOrWhitespace())
		{
			LastText = AddText(FText::Format(LOCTEXT("Location", "Location: {0}"), Objective.Location), FLinearColor::White, 4.f);
		}
		LastText->SetPadding(FMargin(12.f, 4.f, 12.f, 12.f));
	}
}

#undef LOCTEXT_NAMESPACE
