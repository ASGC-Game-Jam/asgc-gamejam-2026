// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputBaseTypes.h"
#include "AtlantisBallastIndicatorWidget.generated.h"

class UCommonInputSubsystem;

/** Shows ballast selection hints for the owning player's Common Input method. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisBallastIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void UpdateInputHints(ECommonInputType InputType);

	TWeakObjectPtr<UCommonInputSubsystem> InputSubsystem;
	FDelegateHandle InputMethodChangedHandle;
};