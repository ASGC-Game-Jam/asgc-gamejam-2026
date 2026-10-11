// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisJournalObjectivesWidget.generated.h"

class UAtlantisJournalObjectivesSubsystem;
class UVerticalBox;

/** Read-only Objectives tab. The Journal shell owns navigation and widget lifetime. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalObjectivesWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnObjectivesChanged();

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> ObjectiveList;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisJournalObjectivesSubsystem> Objectives;
};
