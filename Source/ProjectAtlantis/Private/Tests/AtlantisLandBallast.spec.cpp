// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "Core/AtlantisPlayerController.h"
#include "Core/AtlantisPlayerState.h"
#include "Core/AtlantisOxygenComponent.h"
#include "CommonGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/Script.h"

BEGIN_DEFINE_SPEC(FAtlantisLandBallastSpec, "Atlantis.Input.LandBallast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UGameInstance> Instance;
	TStrongObjectPtr<UCommonGameViewportClient> Viewport;
	AAtlantisPlayerController* Controller = nullptr;
	AAtlantisPlayerState* State = nullptr;
	UEnhancedInputComponent* Input = nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = nullptr;
END_DEFINE_SPEC(FAtlantisLandBallastSpec)

void FAtlantisLandBallastSpec::Define()
{
	BeforeEach([this]()
	{
		Instance.Reset(NewObject<UGameInstance>(GEngine));
		Instance->InitializeStandalone();
		Viewport.Reset(NewObject<UCommonGameViewportClient>(GEngine));
		Instance->GetWorldContext()->GameViewport = Viewport.Get();
		Viewport->Init(*Instance->GetWorldContext(), Instance.Get(), false);
		FString Error;
		ULocalPlayer* Player = Instance->CreateLocalPlayer(0, Error, false);
		UWorld* World = Instance->GetWorld();
		UClass* Class = LoadClass<AAtlantisPlayerController>(nullptr, TEXT("/Game/ProjectAtlantis/Core/PlayerControllers/BP_AtlantisPlayerController.BP_AtlantisPlayerController_C"));
		Controller = World->SpawnActor<AAtlantisPlayerController>(Class);
		Controller->SetPlayer(Player);
		World->AddController(Controller);
		State = World->SpawnActor<AAtlantisPlayerState>();
		Controller->PlayerState = State;
		Controller->Possess(World->SpawnActor<APawn>());
		Input = NewObject<UEnhancedInputComponent>(Controller);
		Controller->InputComponent = Input;
		Controller->SetupInputComponent();
		Subsystem = Player->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		Controller->UpdateMovementControls(false);
		// The isolated world does not run the authored BeginPlay that enables gameplay controls.
		Controller->EnableAllControls();
		TestTrue(TEXT("Fixture has a possessed pawn"), Controller->GetPawn() != nullptr);
		TestTrue(TEXT("Fixture uses expected authoritative state"), Controller->GetPlayerState<AAtlantisPlayerState>() == State && State->HasAuthority());
	});
	AfterEach([this]()
	{
		UWorld* World = Instance->GetWorld();
		Instance->Shutdown();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		Viewport.Reset(); Instance.Reset(); Controller = nullptr; State = nullptr; Input = nullptr; Subsystem = nullptr;
	});
	auto Fire = [this](UInputAction* Action)
	{
		// Isolated worlds have not initialized actors; allow the real server RPC's
		// ProcessEvent path just as the editor's callable-input guard does.
		FEditorScriptExecutionGuard ScriptGuard;
		for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input->GetActionEventBindings())
		{
			if (Binding->GetAction() == Action && Binding->GetTriggerEvent() == ETriggerEvent::Started)
			{
				Binding->Execute(FInputActionInstance(Action));
				return true;
			}
		}
		return false;
	};
	Describe(TEXT("Authored land input and native requests"), [this, Fire]()
	{
		It(TEXT("should map all six fixed Ballast controls on land and preserve Jump"), [this]()
		{
			UInputMappingContext* Land = Controller->DefaultMovementMappingContext;
			TestTrue(TEXT("Actual land context active"), Subsystem->HasMappingContext(Land));
			TestFalse(TEXT("Swimming context inactive"), Subsystem->HasMappingContext(Controller->SwimmingMappingContext));
			const TPair<UInputAction*, FKey> Expected[] = {
				{Controller->DescendBallastAction, EKeys::Two}, {Controller->DescendBallastAction, EKeys::Gamepad_FaceButton_Right},
				{Controller->WanderBallastAction, EKeys::Three}, {Controller->WanderBallastAction, EKeys::Gamepad_FaceButton_Left},
				{Controller->AscendBallastAction, EKeys::Four}, {Controller->AscendBallastAction, EKeys::Gamepad_FaceButton_Top}};
			for (const auto& Pair : Expected)
			{
				TestTrue(*Pair.Value.ToString(), Land->GetMappings().ContainsByPredicate([&Pair](const FEnhancedActionKeyMapping& Mapping)
				{
					return Mapping.Action == Pair.Key && Mapping.Key == Pair.Value;
				}));
			}
			TestTrue(TEXT("Land Space remains Jump"), Land->GetMappings().ContainsByPredicate([](const FEnhancedActionKeyMapping& Mapping)
			{
				return Mapping.Action && Mapping.Action->GetName() == TEXT("IA_Jump") && Mapping.Key == EKeys::SpaceBar;
			}));
			TestFalse(TEXT("No legacy swimming Ballast action on land"), Land->GetMappings().ContainsByPredicate([](const FEnhancedActionKeyMapping& Mapping)
			{
				return Mapping.Action && Mapping.Action->GetName() == TEXT("IA_Ballast");
			}));
		});
		It(TEXT("should dispatch real land action bindings and retain selected state across water transitions"), [this, Fire]()
		{
			TestTrue(TEXT("Wander binding exists"), Fire(Controller->WanderBallastAction));
			TestTrue(TEXT("Wander selected on land"), State->GetBallastState() == EAtlantisBallastState::Wander);
			Fire(Controller->AscendBallastAction);
			TestTrue(TEXT("Ascend selected on land"), State->GetBallastState() == EAtlantisBallastState::Ascend);
			Controller->UpdateMovementControls(true);
			TestTrue(TEXT("Water mapping active"), Subsystem->HasMappingContext(Controller->SwimmingMappingContext));
			TestTrue(TEXT("Selection retained on entry"), State->GetBallastState() == EAtlantisBallastState::Ascend);
			Controller->UpdateMovementControls(false);
			Fire(Controller->DescendBallastAction);
			TestTrue(TEXT("Descend selected after leaving water"), State->GetBallastState() == EAtlantisBallastState::Descend);
		});
		It(TEXT("should preserve Oxygen rejection and suppression for land action callbacks"), [this, Fire]()
		{
			Controller->PushControlSuppression(TEXT("Hiding"));
			Fire(Controller->WanderBallastAction);
			TestTrue(TEXT("Suppression blocks requests"), State->GetBallastState() == EAtlantisBallastState::Descend);
			Controller->PopControlSuppression(TEXT("Hiding"));
			State->GetOxygenComponent()->RequestOxygen(State->GetOxygenComponent()->GetCurrentOxygen() - 1.f);
			Fire(Controller->AscendBallastAction);
			TestTrue(TEXT("Insufficient Oxygen still rejects"), State->GetBallastState() == EAtlantisBallastState::Descend);
			TestEqual(TEXT("Rejected input does not spend Oxygen"), State->GetOxygenComponent()->GetCurrentOxygen(), 1.f);
		});
	});
}
#endif
