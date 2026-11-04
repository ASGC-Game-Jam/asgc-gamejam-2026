// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisJournalObjectivesWidget.generated.h"

class UAtlantisJournalSubsystem;
class UVerticalBox;

/** Read-only Objectives tab. The Journal shell owns navigation and widget lifetime. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisJournalObjectivesWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void OnObjectivesChanged();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> ObjectiveList;

	UPROPERTY(Transient)
	TObjectPtr<UAtlantisJournalSubsystem> Objectives;
};
