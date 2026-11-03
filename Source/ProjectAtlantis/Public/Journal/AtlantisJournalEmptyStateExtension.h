// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "Extensions/UserWidgetExtension.h"
#include "Journal/AtlantisJournalSubsystem.h"
#include "AtlantisJournalEmptyStateExtension.generated.h"

class UButton;
class UTextBlock;

/** Supplies empty-state copy inside the authored Alpha Journal; owns no entry data or pause policy. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalEmptyStateExtension : public UUserWidgetExtension
{
	GENERATED_BODY()
public:
	virtual void Construct() override;
	virtual void Destruct() override;
	/** Called when a native section takes or releases responsibility for its own empty state. */
	UFUNCTION()
	void RefreshEmptyState();

private:
	friend class FAtlantisJournalShellSpec;
	UFUNCTION()
	void OnInvestigationUpdated(EJournalInvestigationStatus OldStatus, EJournalInvestigationStatus Status, const FString& Title);

	UPROPERTY()
	TObjectPtr<UTextBlock> EmptyState;
	UPROPERTY()
	TArray<TObjectPtr<UButton>> TabButtons;
	TWeakObjectPtr<UAtlantisJournalSubsystem> JournalData;
};
