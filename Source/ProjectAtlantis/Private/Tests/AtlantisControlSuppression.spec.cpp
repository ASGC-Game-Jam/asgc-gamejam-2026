// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerController.h"
#include "CommonGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	struct FControlSuppressionFixture
	{
		TStrongObjectPtr<UGameInstance> Instance{NewObject<UGameInstance>(GEngine)};
		TStrongObjectPtr<UCommonGameViewportClient> Viewport;
		AAtlantisPlayerController* Controller = nullptr;
		UEnhancedInputLocalPlayerSubsystem* Input = nullptr;
		ULocalPlayer* LocalPlayer = nullptr;
		TStrongObjectPtr<UInputMappingContext> Land{NewObject<UInputMappingContext>()};
		TStrongObjectPtr<UInputMappingContext> Water{NewObject<UInputMappingContext>()};
		TStrongObjectPtr<UInputMappingContext> Exit{NewObject<UInputMappingContext>()};

		FControlSuppressionFixture()
		{
			Instance->InitializeStandalone();
			Viewport.Reset(NewObject<UCommonGameViewportClient>(GEngine));
			Instance->GetWorldContext()->GameViewport = Viewport.Get();
			Viewport->Init(*Instance->GetWorldContext(), Instance.Get(), false);
			FString Error;
			LocalPlayer = Instance->CreateLocalPlayer(0, Error, false);
			Controller = Instance->GetWorld()->SpawnActor<AAtlantisPlayerController>();
			Controller->SetPlayer(LocalPlayer);
			Instance->GetWorld()->AddController(Controller);
			Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			Controller->AddControls(Land.Get());
		}
		~FControlSuppressionFixture()
		{
			UWorld* World = Instance->GetWorld();
			Instance->RemoveLocalPlayer(LocalPlayer);
			Instance->Shutdown();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisControlSuppressionSpec, "Atlantis.Input.ControlSuppression",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FControlSuppressionFixture> Fixture;
END_DEFINE_SPEC(FAtlantisControlSuppressionSpec)

void FAtlantisControlSuppressionSpec::Define()
{
	BeforeEach([this]() { Fixture = MakeUnique<FControlSuppressionFixture>(); });
	AfterEach([this]() { Fixture.Reset(); });
	Describe(TEXT("composable owner suppression"), [this]()
	{
		It(TEXT("should apply only a temporary exit mapping while preserving desired gameplay contexts"), [this]()
		{
			Fixture->Controller->PushControlSuppression(TEXT("Hiding"), Fixture->Exit.Get());
			TestTrue(TEXT("Suppressed state exposed"), Fixture->Controller->AreGameplayControlsSuppressed());
			TestTrue(TEXT("Desired land controls retained"), Fixture->Controller->GetCurrentMappingContexts().Contains(Fixture->Land.Get()));
			TestFalse(TEXT("Temporary exit is not desired permanently"), Fixture->Controller->GetCurrentMappingContexts().Contains(Fixture->Exit.Get()));
			TestFalse(TEXT("Actual land mapping withdrawn"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
			TestTrue(TEXT("Actual exit mapping applied"), Fixture->Input->HasMappingContext(Fixture->Exit.Get()));
			Fixture->Controller->PopControlSuppression(TEXT("Hiding"));
			TestTrue(TEXT("Land mapping restored"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
			TestFalse(TEXT("Temporary exit removed"), Fixture->Input->HasMappingContext(Fixture->Exit.Get()));
		});
		It(TEXT("should restore current water mappings rather than stale land controls after a suppressed transition"), [this]()
		{
			Fixture->Controller->PushControlSuppression(TEXT("Hiding"), Fixture->Exit.Get());
			Fixture->Controller->RemoveControls(Fixture->Land.Get());
			Fixture->Controller->AddControls(Fixture->Water.Get());
			TestFalse(TEXT("New controls cannot bypass suppression"), Fixture->Input->HasMappingContext(Fixture->Water.Get()));
			Fixture->Controller->PopControlSuppression(TEXT("Hiding"));
			TestTrue(TEXT("Current water mapping restored"), Fixture->Input->HasMappingContext(Fixture->Water.Get()));
			TestFalse(TEXT("Old land mapping stays absent"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
		});
		It(TEXT("should intersect owners and remove only the releasing owner's suppression"), [this]()
		{
			Fixture->Controller->PushControlSuppression(TEXT("Hiding"), Fixture->Exit.Get());
			Fixture->Controller->PushControlSuppression(TEXT("Dialogue"));
			TestFalse(TEXT("Stricter owner also suppresses exit"), Fixture->Input->HasMappingContext(Fixture->Exit.Get()));
			Fixture->Controller->PopControlSuppression(TEXT("Dialogue"));
			TestTrue(TEXT("Hiding exit restored when stricter owner releases"), Fixture->Input->HasMappingContext(Fixture->Exit.Get()));
			TestFalse(TEXT("Gameplay remains suppressed by hiding"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
			Fixture->Controller->PushControlSuppression(TEXT("Dialogue"));
			Fixture->Controller->PopControlSuppression(TEXT("Hiding"));
			TestTrue(TEXT("Other owner's suppression remains"), Fixture->Controller->AreGameplayControlsSuppressed());
			TestFalse(TEXT("Releasing hiding cannot enable gameplay"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
		});
		It(TEXT("should preserve global disabled state and reject anonymous owners"), [this]()
		{
			Fixture->Controller->PushControlSuppression(NAME_None, Fixture->Exit.Get());
			TestFalse(TEXT("Anonymous suppression ignored"), Fixture->Controller->AreGameplayControlsSuppressed());
			Fixture->Controller->PushControlSuppression(TEXT("Hiding"), Fixture->Exit.Get());
			Fixture->Controller->DisableAllControls();
			Fixture->Controller->PopControlSuppression(TEXT("Hiding"));
			TestFalse(TEXT("Global disabled flag retained"), Fixture->Controller->AreControlsEnabled());
			TestFalse(TEXT("Pop cannot enable mappings"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
			Fixture->Controller->EnableAllControls();
			TestTrue(TEXT("Explicit global enable restores desired mappings"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
		});
		It(TEXT("should treat repeated same-owner requests as replacement rather than stacking leaks"), [this]()
		{
			Fixture->Controller->PushControlSuppression(TEXT("Hiding"), Fixture->Exit.Get());
			Fixture->Controller->PushControlSuppression(TEXT("Hiding"));
			TestFalse(TEXT("Replacement can tighten suppression"), Fixture->Input->HasMappingContext(Fixture->Exit.Get()));
			Fixture->Controller->PopControlSuppression(TEXT("Hiding"));
			Fixture->Controller->PopControlSuppression(TEXT("Hiding"));
			TestFalse(TEXT("No leaked suppression"), Fixture->Controller->AreGameplayControlsSuppressed());
			TestTrue(TEXT("One pop restores gameplay"), Fixture->Input->HasMappingContext(Fixture->Land.Get()));
		});
	});
}
#endif
