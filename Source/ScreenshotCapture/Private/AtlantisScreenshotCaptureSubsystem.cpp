// Copyright (c) 2026 ASGC

#include "AtlantisScreenshotCaptureSubsystem.h"
#include "AtlantisScreenshotCaptureSettings.h"
#include "ScreenshotCaptureSettings.h"

#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisScreenshotCapture, Log, All);

namespace
{
	class FAtlantisScreenshotCaptureInputProcessor : public IInputProcessor
	{
	public:
		explicit FAtlantisScreenshotCaptureInputProcessor(UAtlantisScreenshotCaptureSubsystem* InSubsystem)
			: Subsystem(InSubsystem)
		{
		}

		virtual void Tick(float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& KeyEvent) override
		{
			UAtlantisScreenshotCaptureSubsystem* CaptureSubsystem = Subsystem.Get();
			const FKey& CaptureKey = GetDefault<UAtlantisScreenshotCaptureSettings>()->CaptureKey;
			if (!CaptureSubsystem || !CaptureKey.IsValid() || KeyEvent.GetKey() != CaptureKey)
			{
				return false;
			}

			UGameViewportClient* ViewportClient = CaptureSubsystem->GetGameInstance()->GetGameViewportClient();
			const TSharedPtr<SViewport> ViewportWidget = ViewportClient
				                                             ? ViewportClient->GetGameViewportWidget()
				                                             : nullptr;
			const TSharedPtr<SWidget> FocusedWidget = SlateApp.GetUserFocusedWidget(KeyEvent.GetUserIndex());
			if (!ViewportWidget || !FocusedWidget
				|| SlateApp.FindWidgetWindow(ViewportWidget.ToSharedRef()) != SlateApp.FindWidgetWindow(
					FocusedWidget.ToSharedRef()))
			{
				return false;
			}

			// UI-only focus can belong to a menu overlay rather than SViewport itself. Use its
			// screen position to select the correct PIE instance without intercepting editor panels.
			const FGeometry& FocusGeometry = FocusedWidget->GetCachedGeometry();
			const FVector2D FocusCenter = FocusGeometry.LocalToAbsolute(FocusGeometry.GetLocalSize() * 0.5f);
			if (!ViewportWidget->HasAnyUserFocusOrFocusedDescendants()
				&& !ViewportWidget->GetCachedGeometry().IsUnderLocation(FocusCenter))
			{
				return false;
			}

			if (!KeyEvent.IsRepeat())
			{
				CaptureSubsystem->CaptureScreenshot();
			}
			return true;
		}

	private:
		TWeakObjectPtr<UAtlantisScreenshotCaptureSubsystem> Subsystem;
	};
}

bool UAtlantisScreenshotCaptureSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer) && !IsRunningDedicatedServer()
		&& ProjectAtlantis::ScreenshotCapture::IsEnabled(UE_BUILD_SHIPPING != 0, FCommandLine::Get());
}

void UAtlantisScreenshotCaptureSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (!IsRunningDedicatedServer() && FSlateApplication::IsInitialized())
	{
		CaptureInputProcessor = MakeShared<FAtlantisScreenshotCaptureInputProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(CaptureInputProcessor, 0);
	}
}

void UAtlantisScreenshotCaptureSubsystem::Deinitialize()
{
	if (CaptureInputProcessor && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(CaptureInputProcessor);
	}
	CaptureInputProcessor.Reset();
	Super::Deinitialize();
}

struct UAtlantisScreenshotCaptureSubsystem::FCaptureContext
{
	FDateTime CaptureTime;
	FString LevelName;
	FString PlayerName;
	FString Position;
	FString ScreenshotPath;
	FString MetadataPath;
};

void UAtlantisScreenshotCaptureSubsystem::CaptureScreenshot()
{
	// Enforce the launch option for direct Blueprint/C++ callers as well as subsystem creation.
	if (!ProjectAtlantis::ScreenshotCapture::IsEnabled(UE_BUILD_SHIPPING != 0, FCommandLine::Get()))
	{
		return;
	}

	const UGameViewportClient* ViewportClient = GetCaptureViewport();
	if (!ViewportClient)
	{
		return;
	}
	if (FScreenshotRequest::IsScreenshotRequested() || GIsHighResScreenshot || GIsDumpingMovie)
	{
		UE_LOG(LogAtlantisScreenshotCapture, Warning,
		       TEXT("Screenshot capture skipped: another screenshot is pending."));
		return;
	}
	const bool bHdrScreenshot = ViewportClient->Viewport->GetSceneHDREnabled();
	const FCaptureContext Capture = CreateCaptureContext(bHdrScreenshot);
	if (!SaveCaptureMetadata(Capture))
	{
		return;
	}
	// Use the same capture/save path as "shot showui", preserving our matching filenames.
	FScreenshotRequest::RequestScreenshot(Capture.ScreenshotPath, true, false, bHdrScreenshot);
	UE_LOG(LogAtlantisScreenshotCapture, Log, TEXT("Screenshot requested: %s | Level=%s | %s | Metadata=%s"),
	       *Capture.ScreenshotPath, *Capture.LevelName, *Capture.Position, *Capture.MetadataPath);
}

const UGameViewportClient* UAtlantisScreenshotCaptureSubsystem::GetCaptureViewport() const
{
	const UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();
	if (!FSlateApplication::IsInitialized() || !ViewportClient || !ViewportClient->Viewport || !GetWorld())
	{
		UE_LOG(LogAtlantisScreenshotCapture, Warning, TEXT("Screenshot capture requires a game viewport."));
		return nullptr;
	}
	return ViewportClient;
}

UAtlantisScreenshotCaptureSubsystem::FCaptureContext UAtlantisScreenshotCaptureSubsystem::CreateCaptureContext(bool bHdrScreenshot) const
{
	FCaptureContext Capture;
	Capture.CaptureTime = FDateTime::UtcNow();
	Capture.LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	const APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
	const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	Capture.PlayerName = PlayerPawn ? PlayerPawn->GetName() : TEXT("None");
	Capture.Position = TEXT("Unavailable (no possessed player pawn)");

	if (PlayerPawn)
	{
		const FVector Location = PlayerPawn->GetActorLocation();
		Capture.Position = FString::Printf(TEXT("X=%.3f Y=%.3f Z=%.3f"), Location.X, Location.Y, Location.Z);
	}

	// Match FScreenshotRequest, including any project override of the engine's screenshot directory.
	const FString& ScreenshotDirectory = GetDefault<UEngine>()->GameScreenshotSaveDirectory.Path;
	const FString CaptureDirectory = FPaths::ConvertRelativePathToFull(
		ScreenshotDirectory.IsEmpty() ? FPaths::ScreenShotDir() : ScreenshotDirectory);
	const FString CaptureName = FString::Printf(TEXT("Screenshot_%s_%s"),
	                                            *Capture.CaptureTime.ToString(TEXT("%Y%m%d_%H%M%S")),
	                                            *FGuid::NewGuid().ToString(EGuidFormats::Digits));
	Capture.ScreenshotPath = CaptureDirectory / (CaptureName + (bHdrScreenshot ? TEXT(".exr") : TEXT(".png")));
	Capture.MetadataPath = CaptureDirectory / (CaptureName + TEXT(".txt"));
	return Capture;
}

bool UAtlantisScreenshotCaptureSubsystem::SaveCaptureMetadata(const FCaptureContext& Capture)
{
	const FString CaptureDirectory = FPaths::GetPath(Capture.MetadataPath);
	if (!IFileManager::Get().MakeDirectory(*CaptureDirectory, true))
	{
		UE_LOG(LogAtlantisScreenshotCapture, Error, TEXT("Could not create screenshot capture directory: %s"),
		       *CaptureDirectory);
		return false;
	}

	const FString Metadata = FString::Printf(
		TEXT("UTC: %s\nLevel: %s\nPlayer: %s\nPosition (world space, centimeters): %s\nScreenshot: %s\n")
		TEXT("Position sampled at request time; Unreal captures the image and UI at the end of the frame.\n"),
		*Capture.CaptureTime.ToIso8601(), *Capture.LevelName, *Capture.PlayerName, *Capture.Position,
		*Capture.ScreenshotPath);

	if (FFileHelper::SaveStringToFile(Metadata, *Capture.MetadataPath,
	                                  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return true;
	}

	UE_LOG(LogAtlantisScreenshotCapture, Error, TEXT("Could not write screenshot capture metadata: %s"),
	       *Capture.MetadataPath);
	return false;
}
