// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AtlantisBallastIndicatorWidget.generated.h"

class AAtlantisPlayerController;

/** Shows ballast selection hints for the owning player's last active input device. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisBallastIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void UpdateInputHints(bool bUsingGamepad);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	TWeakObjectPtr<AAtlantisPlayerController> InputController;
};