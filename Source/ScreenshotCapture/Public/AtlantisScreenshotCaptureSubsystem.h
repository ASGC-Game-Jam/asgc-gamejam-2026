// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AtlantisScreenshotCaptureSubsystem.generated.h"

class IInputProcessor;
class UGameViewportClient;

/** Screenshot capture survives map travel and does not depend on a controller class or possessed pawn. */
UCLASS()
class SCREENSHOTCAPTURE_API UAtlantisScreenshotCaptureSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** Saves the game viewport including UI and matching metadata to Unreal's screenshot directory. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|ScreenshotCapture")
	void CaptureScreenshot();

private:
	struct FCaptureContext;

	const UGameViewportClient* GetCaptureViewport() const;
	FCaptureContext CreateCaptureContext(bool bHdrScreenshot) const;
	static bool SaveCaptureMetadata(const FCaptureContext& Capture);

	TSharedPtr<IInputProcessor> CaptureInputProcessor;
};
