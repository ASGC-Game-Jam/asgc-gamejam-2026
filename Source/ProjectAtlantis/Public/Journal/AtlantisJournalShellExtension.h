// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "Extensions/UserWidgetExtension.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Ticker.h"
#include "AtlantisJournalShellExtension.generated.h"

class UButton;
class UWidget;

/** Adds Objectives to the existing Blueprint shell without modifying its asset or pause/input logic. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalShellExtension : public UUserWidgetExtension
{
	GENERATED_BODY()
public:
	virtual void Construct() override;
	virtual void Destruct() override;
	virtual bool RequiresTick() const override { return true; }
	virtual void Tick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void OnObjectivesClicked();
	UFUNCTION()
	void OnLegacyTabClicked();
	void ShowLegacyContent();
	int64 GetCurrentTab() const;
	void SetCurrentTab(int64 Tab);
	void RefreshKnowledgeView();

	UPROPERTY(Transient)
	TObjectPtr<UButton> ObjectivesButton;
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ObjectivesView;
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> KnowledgeView;
	UPROPERTY(Transient)
	TObjectPtr<UWidget> LegacyContent;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> LegacyButtons;
	bool bShowingObjectives = false;
	int64 PreviousTab = 0;
	int64 LastKnowledgeTab = INDEX_NONE;
	ESlateVisibility PreviousContentVisibility = ESlateVisibility::Visible;
};

/** Discovers Journal instances created by the existing player-controller Blueprint. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalShellExtensionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
private:
	bool OnDiscoverJournal(float DeltaTime);
	FTSTicker::FDelegateHandle DiscoveryTicker;
};
