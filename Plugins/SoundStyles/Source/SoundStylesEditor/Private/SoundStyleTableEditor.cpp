// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleTableEditor.h"

#include "SoundStyleTable.h"
#include "SoundStyleTags.h"
#include "GameplayTagsManager.h"
#include "Sound/SoundBase.h"
#include "SoundControlBusMix.h"

#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "SGameplayTagCombo.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SNullWidget.h"
#include "AudioParameter.h"

#define LOCTEXT_NAMESPACE "FSoundStyleTableEditor"

// Name tags for the tabs. Register, the layout and the spawners all use them.
static const FName DetailsTabId(TEXT("SoundStyleTableEditor_Details"));
static const FName ClassesTabId(TEXT("SoundStyleTableEditor_Classes"));
static const FName EventsTabId(TEXT("SoundStyleTableEditor_Events"));

// Column IDs for the Events table.
static const FName ColumnEvent(TEXT("Event"));
static const FName ColumnSound(TEXT("Sound"));
static const FName ColumnParams(TEXT("Params"));
static const FName ColumnFrom(TEXT("From"));
static const FName ColumnActions(TEXT("Actions"));

// The table asset's colour (see UAssetDefinition_SoundStyleTable): marks what the selected class sets itself.
static FLinearColor OverrideColor()
{
	return FLinearColor(FColor(220, 191, 255));
}

// ---------------------------------------------------------------------------------------------
// One row of the Events table: Event | Sound picker | From | Silence / Reset buttons.
// Events the selected class sets itself are tinted, with an accent bar and bold name; inherited ones are plain.
// The list asks the editor for a row (GenerateEventRow), the editor makes one of these,
// and the list then asks this row for one widget per column (GenerateWidgetForColumn).
// ---------------------------------------------------------------------------------------------
class SSoundStyleEventRowWidget : public SMultiColumnTableRow<TSharedPtr<FSoundStyleEventRow>>
{
public:
	SLATE_BEGIN_ARGS(SSoundStyleEventRowWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTable, TSharedPtr<FSoundStyleEventRow> InRow, FSoundStyleTableEditor* InEditor)
	{
		Row = InRow;
		Editor = InEditor;
		FSuperRowType::Construct(FTableRowArgs(), OwnerTable);
	}

	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		// Every cell sits on the same tint, so a set-on-this-class event reads as one highlighted band across the table.
		FLinearColor Tint = OverrideColor();
		Tint.A = Row->bThisRow ? 0.12f : 0.f;

		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(Tint)
			.Padding(0.f)
			.VAlign(VAlign_Center)
			[
				BuildCell(ColumnName)
			];
	}

private:
	TSharedRef<SWidget> BuildCell(const FName& ColumnName)
	{
		if (ColumnName == ColumnEvent)
		{
			// Accent bar + bold name when this class sets the event.
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush("WhiteBrush"))
					.BorderBackgroundColor(OverrideColor())
					.Padding(FMargin(1.5f, 0.f))
					.Visibility(Row->bThisRow ? EVisibility::Visible : EVisibility::Hidden)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.Padding(FMargin(6.f, 2.f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Row->Event))
					.Font(Row->bThisRow ? FAppStyle::GetFontStyle("NormalFontBold") : FAppStyle::GetFontStyle("NormalFont"))
				];
		}

		if (ColumnName == ColumnSound)
		{
			// Always fully visible (it's editable). The From column says whether the sound is inherited;
			// picking a sound here overrides it on the selected class.
			return SNew(SBox)
				.Padding(FMargin(2.f))
				.VAlign(VAlign_Center)
				.ToolTipText(Row->bThisRow
					? LOCTEXT("SoundThisRowTip", "Set on this class.")
					: LOCTEXT("SoundInheritedTip", "Inherited. Pick a sound to override it on this class."))
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(USoundBase::StaticClass())
					.ObjectPath(Row->SoundPath)
					.AllowClear(false)               // use Silence / Reset instead, so the meaning is explicit
					.DisplayThumbnail(false)
					.OnObjectChanged(this, &SSoundStyleEventRowWidget::OnSoundPicked)
				];
		}

		if (ColumnName == ColumnParams)
		{
			// Names only; select the event to edit them in the Parameters panel below.
			return SNew(SBox)
				.Padding(FMargin(6.f, 2.f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Row->Params.IsEmpty() ? TEXT("-") : Row->Params))
					.ColorAndOpacity(Row->bThisRow ? FSlateColor::UseForeground() : FSlateColor::UseSubduedForeground())
				];
		}

		if (ColumnName == ColumnFrom)
		{
			return SNew(SBox)
				.Padding(FMargin(6.f, 2.f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Row->From))
					.ColorAndOpacity(Row->bThisRow ? FSlateColor(OverrideColor()) : FSlateColor::UseSubduedForeground())
				];
		}

		// Actions
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Silence", "Silence"))
				.ToolTipText(LOCTEXT("SilenceTip", "Play nothing for this event on this class (stops inheriting)."))
				.Visibility((Row->bThisRow && Row->bSilent) ? EVisibility::Collapsed : EVisibility::Visible)
				.OnClicked(this, &SSoundStyleEventRowWidget::OnSilenceClicked)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(2.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Reset", "Reset"))
				.ToolTipText(LOCTEXT("ResetTip", "Remove this event (sound and parameters) from this class, so it inherits again."))
				.Visibility(Row->bThisRow ? EVisibility::Visible : EVisibility::Collapsed)
				.OnClicked(this, &SSoundStyleEventRowWidget::OnResetClicked)
			];
	}

	void OnSoundPicked(const FAssetData& Asset)
	{
		Editor->SetEventSound(Row->EventTag, Cast<USoundBase>(Asset.GetAsset()));
	}

	FReply OnSilenceClicked()
	{
		Editor->SilenceEvent(Row->EventTag);
		return FReply::Handled();
	}

	FReply OnResetClicked()
	{
		Editor->ResetEvent(Row->EventTag);
		return FReply::Handled();
	}

	TSharedPtr<FSoundStyleEventRow> Row;
	FSoundStyleTableEditor* Editor = nullptr;   // owns the tab this row lives in, so it outlives the row
};

// ---------------------------------------------------------------------------------------------
// Toolkit identity
// ---------------------------------------------------------------------------------------------

FName FSoundStyleTableEditor::GetToolkitFName() const
{
	return FName(TEXT("SoundStyleTableEditor"));
}

FText FSoundStyleTableEditor::GetBaseToolkitName() const
{
	return LOCTEXT("AppLabel", "Sound Style Table Editor");
}

FString FSoundStyleTableEditor::GetWorldCentricTabPrefix() const
{
	return LOCTEXT("WorldCentricTabPrefix", "Sound Style ").ToString();
}

FLinearColor FSoundStyleTableEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.5f, 0.0f, 0.0f, 0.5f);
}

// ---------------------------------------------------------------------------------------------
// Tabs and layout
// ---------------------------------------------------------------------------------------------

void FSoundStyleTableEditor::RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceMenu_SoundStyleTableEditor", "Sound Style Table Editor"));

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner( ClassesTabId, FOnSpawnTab::CreateSP(this, &FSoundStyleTableEditor::SpawnClassesTab) )
		.SetDisplayName( LOCTEXT("ClassesTab", "Rows") )
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner( EventsTabId, FOnSpawnTab::CreateSP(this, &FSoundStyleTableEditor::SpawnEventsTab) )
		.SetDisplayName( LOCTEXT("EventsTab", "Class") )
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	InTabManager->RegisterTabSpawner( DetailsTabId, FOnSpawnTab::CreateSP(this, &FSoundStyleTableEditor::SpawnDetailsTab) )
		.SetDisplayName( LOCTEXT("PropertiesTab", "Details (raw)") )
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
}

void FSoundStyleTableEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner( ClassesTabId );
	InTabManager->UnregisterTabSpawner( EventsTabId );
	InTabManager->UnregisterTabSpawner( DetailsTabId );
}

void FSoundStyleTableEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, USoundStyleTable* Table)
{
	EditedTable = Table;
	Table->SetFlags(RF_Transactional);   // so edits can be undone

	// 0. The raw Details panel (closed by default). If it changes the table, refresh our panes.
	FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(Table);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &FSoundStyleTableEditor::OnDetailsChanged);

	// 0b. The rows list's items (the list widget is built later, in SpawnClassesTab).
	RebuildClassRows();

	// 1. Where the tabs go. Bump _vN whenever you change the layout (Unreal saves each user's layout under this name).
	//    Area (left | right)
	//    ├── Stack: Rows
	//    └── Stack: Class (+ Details, closed; open it from the Window menu)
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("SoundStyleTableEditor_Layout_v4")
	->AddArea
	(
		FTabManager::NewPrimaryArea()
		->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.3f)
			->AddTab(ClassesTabId, ETabState::OpenedTab)
		)
		->Split
		(
			FTabManager::NewStack()
			->SetSizeCoefficient(0.7f)
			->AddTab(EventsTabId, ETabState::OpenedTab)
			->AddTab(DetailsTabId, ETabState::ClosedTab)
		)
	);

	// 2. Open the window with that layout, editing this table.
	const bool bCreateDefaultStandaloneMenu = true;
	const bool bCreateDefaultToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, FName(TEXT("SoundStyleTableEditorApp")), Layout, bCreateDefaultStandaloneMenu, bCreateDefaultToolbar, Table);
}

TSharedRef<SDockTab> FSoundStyleTableEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTab", "Details (raw)"))
		[
			DetailsView.ToSharedRef()
		];
}

// ---------------------------------------------------------------------------------------------
// Left: the rows list
// ---------------------------------------------------------------------------------------------

TSharedRef<SDockTab> FSoundStyleTableEditor::SpawnClassesTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(LOCTEXT("ClassesTabLabel", "Rows"))
		[
			SNew(SVerticalBox)

			// [+ Add Row]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(4.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("AddRow", "+ Add Row"))
					.ToolTipText(LOCTEXT("AddRowTip", "Adds an empty row. Pick its class in the row (the + in the picker creates a new class tag)."))
					.OnClicked_Lambda([this]()
					{
						AddRow();
						return FReply::Handled();
					})
				]
			]

			// One line per row
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(ClassList, SListView<TSharedPtr<FGameplayTag>>)
				.ListItemsSource(&ClassRowItems)
				.OnGenerateRow(this, &FSoundStyleTableEditor::GenerateClassRow)
				.OnSelectionChanged(this, &FSoundStyleTableEditor::OnClassRowSelected)
			]
		];

	// Start with the first row (usually the UISound.Class default) selected, so the right pane isn't empty.
	if (ClassRowItems.Num() > 0)
	{
		ClassList->SetSelection(ClassRowItems[0]);
	}

	return Tab;
}

void FSoundStyleTableEditor::RebuildClassRows()
{
	ClassRowItems.Reset();

	if (EditedTable.IsValid())
	{
		// Sorted by name, so children sit under their parents (Menu, Menu.Back, Pause...). A new row (no class) goes last.
		TArray<FGameplayTag> Keys;
		EditedTable->Classes.GetKeys(Keys);
		Keys.Sort([](const FGameplayTag& A, const FGameplayTag& B)
		{
			if (A.IsValid() != B.IsValid())
			{
				return A.IsValid();
			}
			return A.ToString() < B.ToString();
		});

		for (const FGameplayTag& Key : Keys)
		{
			ClassRowItems.Add(MakeShared<FGameplayTag>(Key));
		}
	}

	if (ClassList.IsValid())
	{
		ClassList->RequestListRefresh();

		// The items are new, so re-select the row that was selected.
		if (bRowSelected)
		{
			SelectRow(SelectedClass);
		}
	}
}

TSharedRef<ITableRow> FSoundStyleTableEditor::GenerateClassRow(TSharedPtr<FGameplayTag> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	const FGameplayTag Tag = *Item;
	const bool bIsRoot = (Tag == FGameplayTag(SoundStyles_Class));

	// Indent by how many parent ROWS it has in this table (not raw tag depth), so it lines up under the row it
	// really inherits from: with no Menu row, Menu.Back sits directly under the default, beside Journal.
	int32 Depth = 0;
	if (Tag.IsValid() && EditedTable.IsValid())
	{
		for (FGameplayTag Parent = Tag.RequestDirectParent(); Parent.IsValid(); Parent = Parent.RequestDirectParent())
		{
			if (EditedTable->Classes.Contains(Parent))
			{
				Depth++;
			}
		}
	}

	// How many events this row sets itself (everything else it inherits).
	int32 SetCount = 0;
	if (const FSoundStyleClass* ClassRow = EditedTable.IsValid() ? EditedTable->Classes.Find(Tag) : nullptr)
	{
		SetCount = ClassRow->Events.Num();
	}

	return SNew(STableRow<TSharedPtr<FGameplayTag>>, OwnerTable)
		.Padding(FMargin(2.f))
		[
			SNew(SHorizontalBox)

			// indent
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SSpacer).Size(FVector2D(Depth * 16.f, 1.f))
			]

			// [ class tag picker ]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SGameplayTagCombo)
				.Filter(TEXT("UISound.Class"))
				.Tag(Tag)
				.OnTagChanged_Lambda([this, Tag](const FGameplayTag NewTag)
				{
					ChangeRowClass(Tag, NewTag);
				})
			]

			// "2 set": how many events this row overrides
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(6.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("SetCount", "{0} set"), FText::AsNumber(SetCount)))
				.ToolTipText(LOCTEXT("SetCountTip", "Events this row sets itself. Everything else is inherited from its parent."))
				.ColorAndOpacity(SetCount > 0 ? FSlateColor(OverrideColor()) : FSlateColor::UseSubduedForeground())
			]

			// (default)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(4.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DefaultLabel", "(default)"))
				.ToolTipText(LOCTEXT("DefaultTip", "The root row: the game-wide default every class falls back to."))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				.Visibility(bIsRoot ? EVisibility::Visible : EVisibility::Collapsed)
			]

			// delete
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), "SimpleButton")
				.ToolTipText(LOCTEXT("DeleteRowTip", "Delete this row and its sounds."))
				.OnClicked_Lambda([this, Tag]()
				{
					DeleteRow(Tag);
					return FReply::Handled();
				})
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush("Icons.Delete"))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		];
}

void FSoundStyleTableEditor::OnClassRowSelected(TSharedPtr<FGameplayTag> Item, ESelectInfo::Type SelectInfo)
{
	bRowSelected = Item.IsValid();
	SelectedClass = Item.IsValid() ? *Item : FGameplayTag();
	RefreshEventRows();
}

void FSoundStyleTableEditor::SelectRow(FGameplayTag ClassTag)
{
	if (!ClassList.IsValid())
	{
		return;
	}

	for (const TSharedPtr<FGameplayTag>& Item : ClassRowItems)
	{
		if (*Item == ClassTag)
		{
			ClassList->SetSelection(Item);
			ClassList->RequestScrollIntoView(Item);
			return;
		}
	}

	// Not in the list any more (e.g. deleted): nothing selected.
	ClassList->ClearSelection();
}

// ---------------------------------------------------------------------------------------------
// Row edits
// ---------------------------------------------------------------------------------------------

void FSoundStyleTableEditor::AddRow()
{
	USoundStyleTable* Table = EditedTable.Get();
	if (!Table)
	{
		return;
	}

	// A map can only hold one empty key: if there's already a new row waiting for its class, go to it.
	if (!Table->Classes.Contains(FGameplayTag()))
	{
		const FScopedTransaction Transaction(LOCTEXT("AddRowTransaction", "Add Sound Style Row"));
		Table->Modify();
		Table->Classes.Add(FGameplayTag(), FSoundStyleClass());
		Table->PostEditChange();
	}

	bRowSelected = true;
	SelectedClass = FGameplayTag();
	OnTableChanged();
}

void FSoundStyleTableEditor::DeleteRow(FGameplayTag ClassTag)
{
	USoundStyleTable* Table = EditedTable.Get();
	if (!Table || !Table->Classes.Contains(ClassTag))
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteRowTransaction", "Delete Sound Style Row"));
	Table->Modify();
	Table->Classes.Remove(ClassTag);
	Table->PostEditChange();

	if (bRowSelected && SelectedClass == ClassTag)
	{
		bRowSelected = false;
		SelectedClass = FGameplayTag();
	}
	OnTableChanged();
}

void FSoundStyleTableEditor::ChangeRowClass(FGameplayTag OldTag, FGameplayTag NewTag)
{
	USoundStyleTable* Table = EditedTable.Get();
	// An empty NewTag is the picker's clear (X) button: the row becomes unassigned (keeps its sounds) until a class is picked.
	if (!Table || NewTag == OldTag || !Table->Classes.Contains(OldTag))
	{
		return;
	}

	if (Table->Classes.Contains(NewTag))
	{
		// A map holds one empty key, so only one unassigned row at a time.
		const FText Message = NewTag.IsValid()
			? FText::Format(LOCTEXT("ClassTaken", "{0} already has a row."), FText::FromString(NewTag.ToString()))
			: LOCTEXT("UnassignedTaken", "There's already a row without a class. Give that one a class first.");
		FNotificationInfo Info(Message);
		Info.ExpireDuration = 4.f;
		FSlateNotificationManager::Get().AddNotification(Info);
		OnTableChanged();   // put the picker back to the row's real class
		return;
	}

	// Move the row's contents to the new key.
	const FScopedTransaction Transaction(LOCTEXT("ChangeRowClassTransaction", "Change Sound Style Row Class"));
	Table->Modify();
	FSoundStyleClass Contents = Table->Classes.FindChecked(OldTag);
	Table->Classes.Remove(OldTag);
	Table->Classes.Add(NewTag, MoveTemp(Contents));
	Table->PostEditChange();

	bRowSelected = true;
	SelectedClass = NewTag;
	OnTableChanged();
}

// ---------------------------------------------------------------------------------------------
// Right: the selected row (heading, While Open Mix, Events table)
// ---------------------------------------------------------------------------------------------

TSharedRef<SDockTab> FSoundStyleTableEditor::SpawnEventsTab(const FSpawnTabArgs& Args)
{
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(LOCTEXT("EventsTabLabel", "Class"))
		[
			SNew(SVerticalBox)

			// Legend for the highlighting below
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 6.f, 8.f, 0.f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Legend", "Highlighted events are set on this class. The rest are inherited (see From)."))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]

			// Heading: the selected class
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 4.f, 8.f, 4.f)
			[
				SNew(STextBlock)
				.Font(FAppStyle::GetFontStyle("NormalFontBold"))
				.Text_Lambda([this]()
				{
					if (!bRowSelected)
					{
						return LOCTEXT("NoRow", "Select a row on the left");
					}
					if (!SelectedClass.IsValid())
					{
						return LOCTEXT("NewRow", "New row: pick its class on the left");
					}
					return FText::FromString(SelectedClass.ToString());
				})
			]

			// While Open Mix
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(8.f, 0.f, 8.f, 8.f)
			[
				SNew(SHorizontalBox)
				.IsEnabled(TAttribute<bool>::CreateLambda([this]() { return SelectedClass.IsValid(); }))
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, 8.f, 0.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("WhileOpenMix", "While Open Mix"))
					.ToolTipText(LOCTEXT("WhileOpenMixTip", "Active while a screen of this class is open (Push/Pop UI Audio State), e.g. duck the world under the pause menu."))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SObjectPropertyEntryBox)
					.AllowedClass(USoundControlBusMix::StaticClass())
					.ObjectPath_Lambda([this]() { return GetWhileOpenMixPath(); })
					.AllowClear(true)
					.DisplayThumbnail(false)
					.OnObjectChanged_Lambda([this](const FAssetData& Asset)
					{
						SetWhileOpenMix(Cast<USoundControlBusMix>(Asset.GetAsset()));
					})
				]
			]

			// Events table (top) | Parameters of the selected event (bottom)
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				+ SSplitter::Slot()
				.Value(0.6f)
				[
					SAssignNew(EventList, SListView<TSharedPtr<FSoundStyleEventRow>>)
					.ListItemsSource(&EventRows)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &FSoundStyleTableEditor::GenerateEventRow)
					.OnSelectionChanged(this, &FSoundStyleTableEditor::OnEventRowSelected)
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column(ColumnEvent).DefaultLabel(LOCTEXT("ColEvent", "Event")).FillWidth(0.12f)
						+ SHeaderRow::Column(ColumnSound).DefaultLabel(LOCTEXT("ColSound", "Sound")).FillWidth(0.33f)
						+ SHeaderRow::Column(ColumnParams).DefaultLabel(LOCTEXT("ColParams", "Parameters")).FillWidth(0.15f)
						+ SHeaderRow::Column(ColumnFrom).DefaultLabel(LOCTEXT("ColFrom", "From")).FillWidth(0.22f)
						+ SHeaderRow::Column(ColumnActions).DefaultLabel(FText::GetEmpty()).FillWidth(0.18f)
					)
				]

				+ SSplitter::Slot()
				.Value(0.4f)
				[
					SNew(SScrollBox)
					+ SScrollBox::Slot()
					.Padding(8.f)
					[
						SAssignNew(ParamsBox, SVerticalBox)
					]
				]
			]
		];

	// The Rows tab may have selected a row before this tab existed: fill the events, select one, build its parameters.
	RefreshEventRows();
	return Tab;
}

TSharedRef<ITableRow> FSoundStyleTableEditor::GenerateEventRow(TSharedPtr<FSoundStyleEventRow> Row, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SSoundStyleEventRowWidget, OwnerTable, Row, this);
}

void FSoundStyleTableEditor::RefreshEventRows()
{
	EventRows.Reset();

	TSharedPtr<FGameplayTagNode> EventRoot = UGameplayTagsManager::Get().FindTagNode(FGameplayTag(SoundStyles_Event));

	if (EditedTable.IsValid() && SelectedClass.IsValid() && EventRoot.IsValid())
	{
		// Ask about the selected class only (one level, no scopes): the same lookup the game uses.
		const TArray<FGameplayTagContainer> Levels = { FGameplayTagContainer(SelectedClass) };

		// One row per event tag: Hover, Press, Release, Open, Close (+ any the project adds).
		for (const TSharedPtr<FGameplayTagNode>& EventNode : EventRoot->GetChildTagNodes())
		{
			FGameplayTag RowTag;
			const FGameplayTag EventTag = EventNode->GetCompleteTag();
			const FSoundStyleEvent* Found = EditedTable->Resolve(Levels, EventTag, RowTag);

			TSharedPtr<FSoundStyleEventRow> Row = MakeShared<FSoundStyleEventRow>();
			Row->EventTag = EventTag;
			Row->Event = EventNode->GetSimpleTagName().ToString();

			if (!Found)
			{
				Row->From = TEXT("not set (plays nothing)");
			}
			else
			{
				Row->bSilent = !Found->Sound;
				Row->SoundPath = Found->Sound ? Found->Sound->GetPathName() : FString();
				Row->bThisRow = (RowTag == SelectedClass);

				TArray<FString> ParamNames;
				for (const FAudioParameter& Param : Found->Parameters)
				{
					ParamNames.Add(Param.ParamName.ToString());
				}
				Row->Params = FString::Join(ParamNames, TEXT(", "));

				if (Row->bThisRow)
				{
					// What would play without this override? Ask the parent class (root falls back to nothing).
					Row->From = TEXT("this class");
					const FGameplayTag ParentClass = SelectedClass.RequestDirectParent();
					if (SelectedClass != FGameplayTag(SoundStyles_Class) && ParentClass.IsValid())
					{
						FGameplayTag ParentRowTag;
						const TArray<FGameplayTagContainer> ParentLevels = { FGameplayTagContainer(ParentClass) };
						if (EditedTable->Resolve(ParentLevels, EventTag, ParentRowTag))
						{
							FString OverriddenName = (ParentRowTag == FGameplayTag(SoundStyles_Class)) ? FString(TEXT("default")) : ParentRowTag.ToString();
							OverriddenName.RemoveFromStart(TEXT("UISound.Class."));
							Row->From = FString::Printf(TEXT("this class (overrides %s)"), *OverriddenName);
						}
					}
				}
				else if (RowTag == FGameplayTag(SoundStyles_Class))
				{
					Row->From = TEXT("default");
				}
				else
				{
					FString ParentName = RowTag.ToString();
					ParentName.RemoveFromStart(TEXT("UISound.Class."));
					Row->From = FString::Printf(TEXT("inherited from %s"), *ParentName);
				}

				if (Row->bSilent)
				{
					Row->From += TEXT(" (silent)");
				}
			}

			EventRows.Add(Row);
		}
	}

	// The data changed: tell the list to rebuild its rows.
	if (EventList.IsValid())
	{
		EventList->RequestListRefresh();

		// The items are new, so re-select the same event (or the first one).
		TSharedPtr<FSoundStyleEventRow> ToSelect;
		for (const TSharedPtr<FSoundStyleEventRow>& Row : EventRows)
		{
			if (Row->EventTag == SelectedEvent)
			{
				ToSelect = Row;
				break;
			}
		}
		if (!ToSelect.IsValid() && EventRows.Num() > 0)
		{
			ToSelect = EventRows[0];
		}

		if (ToSelect.IsValid())
		{
			SelectedEvent = ToSelect->EventTag;
			EventList->SetSelection(ToSelect);
		}
		else
		{
			EventList->ClearSelection();
		}
	}

	RefreshParameterPanel();
}

void FSoundStyleTableEditor::OnEventRowSelected(TSharedPtr<FSoundStyleEventRow> Row, ESelectInfo::Type SelectInfo)
{
	// Ignore "nothing selected" (the list clears while rebuilding); keep the last event.
	if (Row.IsValid())
	{
		SelectedEvent = Row->EventTag;
		RefreshParameterPanel();
	}
}

void FSoundStyleTableEditor::RefreshParameterPanel()
{
	if (!ParamsBox.IsValid())
	{
		return;   // Class tab not built yet
	}
	ParamsBox->ClearChildren();

	// A grey line of help text.
	auto AddHint = [this](const FText& Text)
	{
		ParamsBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f)
		[
			SNew(STextBlock)
			.Text(Text)
			.AutoWrapText(true)
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	};

	USoundStyleTable* Table = EditedTable.Get();
	if (!Table || !SelectedClass.IsValid() || !SelectedEvent.IsValid())
	{
		AddHint(LOCTEXT("ParamsNoEvent", "Select an event above to see its parameters."));
		return;
	}

	FString EventName = SelectedEvent.ToString();
	EventName.RemoveFromStart(TEXT("UISound.Event."));

	// Heading: "Parameters: Press"
	ParamsBox->AddSlot()
	.AutoHeight()
	.Padding(0.f, 0.f, 0.f, 4.f)
	[
		SNew(STextBlock)
		.Font(FAppStyle::GetFontStyle("NormalFontBold"))
		.Text(FText::Format(LOCTEXT("ParamsHeading", "Parameters: {0}"), FText::FromString(EventName)))
	];

	// Same lookup the game uses, for this class only.
	const TArray<FGameplayTagContainer> Levels = { FGameplayTagContainer(SelectedClass) };
	FGameplayTag RowTag;
	const FSoundStyleEvent* Found = Table->Resolve(Levels, SelectedEvent, RowTag);

	if (!Found || !Found->Sound)
	{
		AddHint(LOCTEXT("ParamsNoSound", "Pick a sound for this event first: parameters are sent to its sound."));
		return;
	}

	if (RowTag != SelectedClass)
	{
		FString ParentName = RowTag.ToString();
		ParentName.RemoveFromStart(TEXT("UISound.Class."));
		AddHint(FText::Format(LOCTEXT("ParamsInherited", "Inherited from {0}. Editing here overrides {1} on this class (keeps its sound)."),
			FText::FromString(ParentName), FText::FromString(EventName)));
	}

	for (int32 Index = 0; Index < Found->Parameters.Num(); Index++)
	{
		ParamsBox->AddSlot()
		.AutoHeight()
		.Padding(0.f, 2.f)
		[
			BuildParameterLine(Index, Found->Parameters[Index])
		];
	}

	if (Found->Parameters.Num() == 0)
	{
		AddHint(LOCTEXT("ParamsNone", "None: the sound starts with its own defaults."));
	}

	// [+ Add Parameter]
	ParamsBox->AddSlot()
	.AutoHeight()
	.HAlign(HAlign_Left)
	.Padding(0.f, 6.f, 0.f, 4.f)
	[
		SNew(SButton)
		.Text(LOCTEXT("AddParam", "+ Add Parameter"))
		.ToolTipText(LOCTEXT("AddParamTip", "Adds a Float parameter called Value. Rename it to match a MetaSound input."))
		.OnClicked_Lambda([this]()
		{
			EditEvent(SelectedEvent, LOCTEXT("AddParamTransaction", "Add UI Sound Parameter"), [](FSoundStyleEvent& Event)
			{
				FAudioParameter NewParam;
				NewParam.ParamName = TEXT("Value");
				NewParam.ParamType = EAudioParameterType::Float;
				Event.Parameters.Add(NewParam);
			});
			return FReply::Handled();
		})
	];

	AddHint(LOCTEXT("ParamsHelp", "Names must match the MetaSound's inputs exactly (unknown names are ignored). "
		"Sent when the sound starts; game values sent at play time override these by name."));
}

TSharedRef<SWidget> FSoundStyleTableEditor::BuildParameterLine(int32 Index, const FAudioParameter& Param)
{
	// Each callback compares with the value the line was built with, so clicking away without a change makes no undo step.
	const FName OldName = Param.ParamName;
	const EAudioParameterType OldType = Param.ParamType;
	const float OldFloat = Param.FloatParam;
	const int32 OldInt = Param.IntParam;

	// Value: depends on the type
	TSharedRef<SWidget> ValueWidget = SNullWidget::NullWidget;
	switch (Param.ParamType)
	{
	case EAudioParameterType::Float:
		ValueWidget = SNew(SNumericEntryBox<float>)
			.Value(TOptional<float>(Param.FloatParam))
			.OnValueCommitted_Lambda([this, Index, OldFloat](float NewValue, ETextCommit::Type)
			{
				if (NewValue == OldFloat)
				{
					return;
				}
				EditEvent(SelectedEvent, LOCTEXT("SetParamValue", "Set UI Sound Parameter"), [Index, NewValue](FSoundStyleEvent& Event)
				{
					if (Event.Parameters.IsValidIndex(Index))
					{
						Event.Parameters[Index].FloatParam = NewValue;
					}
				});
			});
		break;

	case EAudioParameterType::Integer:
		ValueWidget = SNew(SNumericEntryBox<int32>)
			.Value(TOptional<int32>(Param.IntParam))
			.OnValueCommitted_Lambda([this, Index, OldInt](int32 NewValue, ETextCommit::Type)
			{
				if (NewValue == OldInt)
				{
					return;
				}
				EditEvent(SelectedEvent, LOCTEXT("SetParamValue", "Set UI Sound Parameter"), [Index, NewValue](FSoundStyleEvent& Event)
				{
					if (Event.Parameters.IsValidIndex(Index))
					{
						Event.Parameters[Index].IntParam = NewValue;
					}
				});
			});
		break;

	case EAudioParameterType::Boolean:
		ValueWidget = SNew(SCheckBox)
			.IsChecked(Param.BoolParam ? ECheckBoxState::Checked : ECheckBoxState::Unchecked)
			.OnCheckStateChanged_Lambda([this, Index](ECheckBoxState NewState)
			{
				const bool bNewValue = (NewState == ECheckBoxState::Checked);
				EditEvent(SelectedEvent, LOCTEXT("SetParamValue", "Set UI Sound Parameter"), [Index, bNewValue](FSoundStyleEvent& Event)
				{
					if (Event.Parameters.IsValidIndex(Index))
					{
						Event.Parameters[Index].BoolParam = bNewValue;
					}
				});
			});
		break;

	default:
		// Strings, objects, arrays, Default: rare for UI sounds. Still sent; edit them in the raw Details tab.
		ValueWidget = SNew(STextBlock)
			.Text(LOCTEXT("ParamOtherType", "(other type: edit in Details (raw))"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground());
		break;
	}

	return SNew(SHorizontalBox)

		// Name
		+ SHorizontalBox::Slot()
		.FillWidth(0.35f)
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SEditableTextBox)
			.Text(FText::FromName(Param.ParamName))
			.ToolTipText(LOCTEXT("ParamNameTip", "Must match a MetaSound input name exactly (same capitals)."))
			.OnTextCommitted_Lambda([this, Index, OldName](const FText& NewText, ETextCommit::Type)
			{
				const FName NewName(*NewText.ToString().TrimStartAndEnd());
				if (NewName.IsNone() || NewName == OldName)
				{
					RefreshParameterPanel();   // put the old name back
					return;
				}
				EditEvent(SelectedEvent, LOCTEXT("RenameParam", "Rename UI Sound Parameter"), [Index, NewName](FSoundStyleEvent& Event)
				{
					if (Event.Parameters.IsValidIndex(Index))
					{
						Event.Parameters[Index].ParamName = NewName;
					}
				});
			})
		]

		// Float | Int | Bool
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SSegmentedControl<EAudioParameterType>)
			.Value(Param.ParamType)
			.OnValueChanged_Lambda([this, Index, OldType](EAudioParameterType NewType)
			{
				if (NewType == OldType)
				{
					return;
				}
				EditEvent(SelectedEvent, LOCTEXT("SetParamType", "Set UI Sound Parameter Type"), [Index, NewType](FSoundStyleEvent& Event)
				{
					if (Event.Parameters.IsValidIndex(Index))
					{
						Event.Parameters[Index].ParamType = NewType;
					}
				});
			})
			+ SSegmentedControl<EAudioParameterType>::Slot(EAudioParameterType::Float).Text(LOCTEXT("TypeFloat", "Float"))
			+ SSegmentedControl<EAudioParameterType>::Slot(EAudioParameterType::Integer).Text(LOCTEXT("TypeInt", "Int"))
			+ SSegmentedControl<EAudioParameterType>::Slot(EAudioParameterType::Boolean).Text(LOCTEXT("TypeBool", "Bool"))
		]

		// Value
		+ SHorizontalBox::Slot()
		.FillWidth(0.65f)
		.VAlign(VAlign_Center)
		[
			ValueWidget
		]

		// delete
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), "SimpleButton")
			.ToolTipText(LOCTEXT("DeleteParamTip", "Remove this parameter."))
			.OnClicked_Lambda([this, Index]()
			{
				EditEvent(SelectedEvent, LOCTEXT("RemoveParam", "Remove UI Sound Parameter"), [Index](FSoundStyleEvent& Event)
				{
					if (Event.Parameters.IsValidIndex(Index))
					{
						Event.Parameters.RemoveAt(Index);
					}
				});
				return FReply::Handled();
			})
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush("Icons.Delete"))
				.ColorAndOpacity(FSlateColor::UseForeground())
			]
		];
}

FString FSoundStyleTableEditor::GetWhileOpenMixPath() const
{
	if (!EditedTable.IsValid() || !SelectedClass.IsValid())
	{
		return FString();
	}

	const FSoundStyleClass* Row = EditedTable->Classes.Find(SelectedClass);
	return (Row && Row->WhileOpenMix) ? Row->WhileOpenMix->GetPathName() : FString();
}

// ---------------------------------------------------------------------------------------------
// Event edits. Each one: a named undo step (FScopedTransaction), Modify() to snapshot the table,
// change it, then refresh both panes.
// ---------------------------------------------------------------------------------------------

USoundStyleTable* FSoundStyleTableEditor::BeginEdit()
{
	USoundStyleTable* Table = EditedTable.Get();
	if (!Table || !SelectedClass.IsValid() || !Table->Classes.Contains(SelectedClass))
	{
		return nullptr;
	}
	return Table;
}

void FSoundStyleTableEditor::EditEvent(FGameplayTag EventTag, const FText& UndoName, TFunctionRef<void(FSoundStyleEvent&)> Edit)
{
	USoundStyleTable* Table = BeginEdit();
	if (!Table || !EventTag.IsValid())
	{
		return;
	}

	// Start from what plays now: this class's own entry, else the inherited one (so an override keeps its
	// sound and parameters), else nothing.
	FSoundStyleClass& Row = Table->Classes.FindChecked(SelectedClass);
	FSoundStyleEvent Working;
	if (const FSoundStyleEvent* Own = Row.Events.Find(EventTag))
	{
		Working = *Own;
	}
	else
	{
		const TArray<FGameplayTagContainer> Levels = { FGameplayTagContainer(SelectedClass) };
		FGameplayTag RowTag;
		if (const FSoundStyleEvent* Inherited = Table->Resolve(Levels, EventTag, RowTag))
		{
			Working = *Inherited;
		}
	}

	const FScopedTransaction Transaction(UndoName);
	Table->Modify();
	Edit(Working);
	Row.Events.Add(EventTag, MoveTemp(Working));   // Add replaces the entry if the key is already there
	Table->PostEditChange();
	OnTableChanged();
}

void FSoundStyleTableEditor::SetEventSound(FGameplayTag EventTag, USoundBase* Sound)
{
	if (!Sound)
	{
		return;
	}

	EditEvent(EventTag, LOCTEXT("SetEventSound", "Set UI Sound"), [Sound](FSoundStyleEvent& Event)
	{
		Event.Sound = Sound;
	});
}

void FSoundStyleTableEditor::SilenceEvent(FGameplayTag EventTag)
{
	EditEvent(EventTag, LOCTEXT("SilenceEvent", "Silence UI Sound"), [](FSoundStyleEvent& Event)
	{
		Event.Sound = nullptr;
	});
}

void FSoundStyleTableEditor::ResetEvent(FGameplayTag EventTag)
{
	USoundStyleTable* Table = BeginEdit();
	if (!Table || !Table->Classes.FindChecked(SelectedClass).Events.Contains(EventTag))
	{
		return;   // nothing to reset
	}

	const FScopedTransaction Transaction(LOCTEXT("ResetEvent", "Reset UI Sound"));
	Table->Modify();
	Table->Classes.FindChecked(SelectedClass).Events.Remove(EventTag);
	Table->PostEditChange();
	OnTableChanged();
}

void FSoundStyleTableEditor::SetWhileOpenMix(USoundControlBusMix* Mix)
{
	USoundStyleTable* Table = BeginEdit();
	if (!Table)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("SetWhileOpenMix", "Set While Open Mix"));
	Table->Modify();
	Table->Classes.FindChecked(SelectedClass).WhileOpenMix = Mix;
	Table->PostEditChange();
	OnTableChanged();
}

// ---------------------------------------------------------------------------------------------
// Keeping both panes in sync
// ---------------------------------------------------------------------------------------------

void FSoundStyleTableEditor::OnTableChanged()
{
	RebuildClassRows();   // also re-selects, which refreshes the events
	RefreshEventRows();
}

void FSoundStyleTableEditor::OnDetailsChanged(const FPropertyChangedEvent& Event)
{
	OnTableChanged();
}

void FSoundStyleTableEditor::PostUndo(bool bSuccess)
{
	OnTableChanged();
}

void FSoundStyleTableEditor::PostRedo(bool bSuccess)
{
	OnTableChanged();
}

#undef LOCTEXT_NAMESPACE
