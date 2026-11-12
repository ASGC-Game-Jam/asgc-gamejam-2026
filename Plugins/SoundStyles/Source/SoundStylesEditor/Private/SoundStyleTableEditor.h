// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "EditorUndoClient.h"
#include "GameplayTagContainer.h"

class USoundStyleTable;
class USoundBase;
class USoundControlBusMix;
class IDetailsView;
struct FPropertyChangedEvent;
class ITableRow;
class STableViewBase;
class SVerticalBox;
class SWidget;
struct FSoundStyleEvent;
struct FAudioParameter;

template <typename ItemType> class SListView;

/** One line in the Events table: what one event plays for the selected class. */
struct FSoundStyleEventRow
{
	FGameplayTag EventTag;   // e.g. UISound.Event.Hover
	FString Event;           // e.g. "Hover"
	FString SoundPath;       // the sound that will play (for the picker); empty = none
	FString Params;          // parameter names, e.g. "Value, Brightness"; empty = none
	FString From;            // e.g. "this class", "inherited from Menu", "default", "not set"
	bool bThisRow = false;   // the selected class sets this event itself
	bool bSilent = false;    // the answering row lists the event with no sound
};

/**
 * Double-click editor for a Sound Style Table.
 *   Left:  the table's rows. "+ Add Row" adds an empty row; pick its class tag in the row itself.
 *          Changing a row's tag moves the row to the new class. The bin deletes the row.
 *   Right: the selected row: its While Open Mix, and every event with the sound that will
 *          actually play and where it comes from. Picking a sound sets it on this class.
 *          Below: the selected event's Parameters (name, type, value).
 * Editing an inherited event copies it onto this class first (sound + parameters), then changes it.
 * All edits are undoable.
 */
class FSoundStyleTableEditor : public FAssetEditorToolkit, public FSelfRegisteringEditorUndoClient
{
public:
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	/** Opens the editor window for this table. */
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, USoundStyleTable* Table);

	// --- Row edits (left pane). Each is one undoable step. ---

	/** Adds an empty row (no class yet) and selects it. Pick its class in the row. */
	void AddRow();

	/** Deletes a row and everything in it. */
	void DeleteRow(FGameplayTag ClassTag);

	/**
	 * Moves a row to a different class tag (keeps its sounds and mix). Refuses if that class already has a row.
	 * An empty NewTag (the picker's clear button) makes the row unassigned, if there isn't already an unassigned row.
	 */
	void ChangeRowClass(FGameplayTag OldTag, FGameplayTag NewTag);

	// --- Event edits on the selected row (right pane). Each is one undoable step. ---

	/** Sets this event's sound on the selected class. */
	void SetEventSound(FGameplayTag EventTag, USoundBase* Sound);

	/** Lists this event on the selected class with no sound: silent, stops inheritance. */
	void SilenceEvent(FGameplayTag EventTag);

	/** Removes this event from the selected class, so it inherits again. */
	void ResetEvent(FGameplayTag EventTag);

	/** Sets the selected class's While Open Mix (nullptr clears it). */
	void SetWhileOpenMix(USoundControlBusMix* Mix);

	// --- Undo / redo: refresh both panes ---
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

private:
	/** Builds the Details tab (raw table view; closed by default, open from the Window menu). */
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);

	/** Builds the Rows tab: + Add Row, then one line per row with its class picker. */
	TSharedRef<SDockTab> SpawnClassesTab(const FSpawnTabArgs& Args);

	/** Builds the Class tab: the selected row's mix and events. */
	TSharedRef<SDockTab> SpawnEventsTab(const FSpawnTabArgs& Args);

	/** The raw Details panel. Made in InitEditor, shown by SpawnDetailsTab. */
	TSharedPtr<IDetailsView> DetailsView;

	/** The table being edited. Weak: an F class can't use UPROPERTY; the asset editor keeps it alive. */
	TWeakObjectPtr<USoundStyleTable> EditedTable;

	// --- Left: the rows list ---

	/** One item per row in the table: its class tag (empty for a new row). Sorted, empty last. */
	TArray<TSharedPtr<FGameplayTag>> ClassRowItems;

	/** The rows list widget. */
	TSharedPtr<SListView<TSharedPtr<FGameplayTag>>> ClassList;

	/** Rebuilds ClassRowItems from the table and keeps the selection. */
	void RebuildClassRows();

	/** List view asks: "make a line for this row" (indent, class picker, delete). */
	TSharedRef<ITableRow> GenerateClassRow(TSharedPtr<FGameplayTag> Item, const TSharedRef<STableViewBase>& OwnerTable);

	/** List view says: "the user selected this row". */
	void OnClassRowSelected(TSharedPtr<FGameplayTag> Item, ESelectInfo::Type SelectInfo);

	/** Selects the row for this class tag (empty tag = the new, unassigned row). */
	void SelectRow(FGameplayTag ClassTag);

	/** Is a row selected? (A new row is selected with an empty SelectedClass.) */
	bool bRowSelected = false;

	/** The selected row's class. Empty when nothing is selected, or for a new row with no class yet. */
	FGameplayTag SelectedClass;

	// --- Right: the selected row's events ---

	/** The Events table's rows, rebuilt whenever the selection or the table changes. */
	TArray<TSharedPtr<FSoundStyleEventRow>> EventRows;

	/** The Events table widget, kept so we can tell it to refresh. */
	TSharedPtr<SListView<TSharedPtr<FSoundStyleEventRow>>> EventList;

	/** Fills EventRows for SelectedClass (using the table's Resolve), then refreshes the list. */
	void RefreshEventRows();

	/** List view asks: "make a row for this event". */
	TSharedRef<ITableRow> GenerateEventRow(TSharedPtr<FSoundStyleEventRow> Row, const TSharedRef<STableViewBase>& OwnerTable);

	/** Path of the selected class's While Open Mix (for the picker); empty if none. */
	FString GetWhileOpenMixPath() const;

	/** The table, if the selected row has a class (event edits need one); otherwise nullptr. */
	USoundStyleTable* BeginEdit();

	/**
	 * One undoable edit of one event on the selected class. Starts from this class's own entry,
	 * or a copy of the inherited one (so overriding keeps the sound and parameters), or an empty one.
	 */
	void EditEvent(FGameplayTag EventTag, const FText& UndoName, TFunctionRef<void(FSoundStyleEvent&)> Edit);

	// --- Below the events: the selected event's parameters ---

	/** The event selected in the Events table (kept across refreshes and class changes). */
	FGameplayTag SelectedEvent;

	/** The Parameters panel's contents; rebuilt by RefreshParameterPanel. */
	TSharedPtr<SVerticalBox> ParamsBox;

	/** Events table says: "the user selected this event". */
	void OnEventRowSelected(TSharedPtr<FSoundStyleEventRow> Row, ESelectInfo::Type SelectInfo);

	/** Rebuilds the Parameters panel for SelectedClass + SelectedEvent. */
	void RefreshParameterPanel();

	/** One line of the Parameters panel: Name | Float/Int/Bool | Value | delete. */
	TSharedRef<SWidget> BuildParameterLine(int32 Index, const FAudioParameter& Param);

	/** Something in the table changed (edit, undo, Details panel): refresh both panes. */
	void OnTableChanged();

	/** The raw Details panel changed a property. */
	void OnDetailsChanged(const FPropertyChangedEvent& Event);
};
