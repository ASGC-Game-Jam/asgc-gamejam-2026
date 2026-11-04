// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "AtlantisScreenshotCaptureSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Slate/SceneViewport.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SViewport.h"

namespace
{
	struct FAtlantisScreenshotCaptureRequestFixture
	{
		TStrongObjectPtr<UGameInstance> GameInstance;
		TStrongObjectPtr<UGameViewportClient> ViewportClient;
		TUniquePtr<FSceneViewport> SceneViewport;
		FString OriginalScreenshotDirectory;
		FString CaptureDirectory;
		FString ScreenshotPath;
		UAtlantisScreenshotCaptureSubsystem* Subsystem = nullptr;

		FAtlantisScreenshotCaptureRequestFixture()
			: GameInstance(NewObject<UGameInstance>(GEngine)),
			  ViewportClient(NewObject<UGameViewportClient>(GEngine))
		{
			GameInstance->InitializeStandalone();
			GameInstance->GetWorldContext()->GameViewport = ViewportClient.Get();
			ViewportClient->Init(*GameInstance->GetWorldContext(), GameInstance.Get(), false);
			SceneViewport = MakeUnique<FSceneViewport>(ViewportClient.Get(), SNew(SViewport));
			ViewportClient->Viewport = SceneViewport.Get();
			OriginalScreenshotDirectory = GetDefault<UEngine>()->GameScreenshotSaveDirectory.Path;
			CaptureDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation")
				/ FGuid::NewGuid().ToString(EGuidFormats::Digits));
			GetMutableDefault<UEngine>()->GameScreenshotSaveDirectory.Path = CaptureDirectory;
			Subsystem = GameInstance->GetSubsystem<UAtlantisScreenshotCaptureSubsystem>();
		}

		~FAtlantisScreenshotCaptureRequestFixture()
		{
			FScreenshotRequest::Reset();
			GetMutableDefault<UEngine>()->GameScreenshotSaveDirectory.Path = OriginalScreenshotDirectory;
			if (!ScreenshotPath.IsEmpty())
			{
				IFileManager::Get().Delete(*FPaths::ChangeExtension(ScreenshotPath, TEXT("txt")));
				IFileManager::Get().Delete(*ScreenshotPath);
			}
			IFileManager::Get().DeleteDirectory(*CaptureDirectory);
			ViewportClient->Viewport = nullptr;
			SceneViewport.Reset();
			UWorld* World = GameInstance->GetWorld();
			GameInstance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisScreenshotCaptureRequestSpec, "Atlantis.ScreenshotCapture.Request",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisScreenshotCaptureRequestFixture> Fixture;
END_DEFINE_SPEC(FAtlantisScreenshotCaptureRequestSpec)

void FAtlantisScreenshotCaptureRequestSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisScreenshotCaptureRequestFixture>();
		TestNotNull(TEXT("Capture subsystem created"), Fixture->Subsystem);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("BuiltInCapture"), [this]()
	{
		It(TEXT("should queue an engine screenshot with UI and write matching metadata without a pawn"), [this]()
		{
			if (!Fixture->Subsystem)
			{
				return;
			}
			Fixture->Subsystem->CaptureScreenshot();
			Fixture->ScreenshotPath = FScreenshotRequest::GetFilename();
			TestTrue(TEXT("Unreal owns the pending screenshot request"), FScreenshotRequest::IsScreenshotRequested());
			TestTrue(TEXT("Request includes menus and HUD"), FScreenshotRequest::ShouldShowUI());
			TestEqual(TEXT("Request uses the configured engine directory"), FPaths::GetPath(Fixture->ScreenshotPath), Fixture->CaptureDirectory);
			FString Metadata;
			TestTrue(TEXT("Matching metadata exists before deferred capture"), FFileHelper::LoadFileToString(
				Metadata, *FPaths::ChangeExtension(Fixture->ScreenshotPath, TEXT("txt"))));
			TestTrue(TEXT("Metadata points to the requested image"), Metadata.Contains(Fixture->ScreenshotPath));
			TestTrue(TEXT("Menu capture does not require a pawn"), Metadata.Contains(TEXT("Unavailable (no possessed player pawn)")));
		});
	});
}

#endif
