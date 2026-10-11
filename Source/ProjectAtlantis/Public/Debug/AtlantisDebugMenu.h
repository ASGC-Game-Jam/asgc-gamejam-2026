// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisDebugMenu.generated.h"

/** Native menu with standard Slate button navigation; no binary assets required. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisDebugMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	void FocusFirstAction();
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:
	TSharedPtr<SWidget> FirstAction;
};
