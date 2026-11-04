// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS

#include "Journal/AtlantisJournalEmptyStateExtension.h"
#include "Journal/AtlantisJournalSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "CommonGameViewportClient.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Core/AtlantisPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameModeBase.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Slate/SceneViewport.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Widgets/SViewport.h"
#include "Widgets/SOverlay.h"

namespace
{
	struct FJournalShellFixture
	{
		TStrongObjectPtr<UGameInstance> Instance;
		TStrongObjectPtr<UCommonGameViewportClient> Viewport;
		TSharedPtr<SViewport> ViewportWidget;
		TSharedPtr<SOverlay> ViewportOverlay;
		TUniquePtr<FSceneViewport> SceneViewport;
		AAtlantisPlayerController* Controller = nullptr;
		UUserWidget* Journal = nullptr;

		FJournalShellFixture() : Instance(NewObject<UGameInstance>(GEngine))
		{
			Instance->InitializeStandalone();
			UWorld* World = Instance->GetWorld();
			FURL Url;
			Url.AddOption(TEXT("game=/Script/Engine.GameModeBase"));
			World->SetGameMode(Url);
			World->InitializeActorsForPlay(Url);
			Viewport.Reset(NewObject<UCommonGameViewportClient>(GEngine));
			Instance->GetWorldContext()->GameViewport = Viewport.Get();
			Viewport->Init(*Instance->GetWorldContext(), Instance.Get(), false);
			ViewportWidget = SNew(SViewport);
			SceneViewport.Reset(Viewport->CreateGameViewport(ViewportWidget));
			Viewport->SetViewportFrame(SceneViewport.Get());
			ViewportOverlay = SNew(SOverlay);
			Viewport->SetViewportOverlayWidget(nullptr, ViewportOverlay.ToSharedRef());
			FString Error;
			ULocalPlayer* Player = Instance->CreateLocalPlayer(0, Error, false);
			UClass* ControllerClass = LoadClass<AAtlantisPlayerController>(nullptr,
				TEXT("/Game/ProjectAtlantis/Core/PlayerControllers/BP_AtlantisPlayerController.BP_AtlantisPlayerController_C"));
			Controller = ControllerClass ? World->SpawnActor<AAtlantisPlayerController>(ControllerClass) : nullptr;
			if (!Controller || !Player) { return; }
			Controller->SetPlayer(Player);
			World->AddController(Controller);
			if (!Controller->PlayerState) { Controller->InitPlayerState(); }
			Controller->DispatchBeginPlay();
			if (const FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Controller->GetClass(), TEXT("JournalWidget")))
			{
				Journal = Cast<UUserWidget>(Property->GetObjectPropertyValue_InContainer(Controller));
			}
		}

		bool Toggle()
		{
			if (const UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Controller ? Controller->InputComponent : nullptr))
			{
				for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input->GetActionEventBindings())
				{
					if (Binding->GetAction() && Binding->GetAction()->GetFName() == TEXT("IA_Journal")
						&& Binding->GetTriggerEvent() == ETriggerEvent::Started)
					{
						Binding->Execute(FInputActionInstance(Binding->GetAction()));
						return true;
					}
				}
			}
			return false;
		}
		~FJournalShellFixture()
		{
			if (Journal && Journal->IsInViewport()) { Toggle(); }
			UWorld* World = Instance->GetWorld();
			Instance->Shutdown();
			Viewport->SetViewportFrame(nullptr);
			SceneViewport.Reset();
			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisJournalShellSpec, "Atlantis.Journal.AlphaShell",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FJournalShellFixture> Fixture;
END_DEFINE_SPEC(FAtlantisJournalShellSpec)

void FAtlantisJournalShellSpec::Define()
{
	Describe(TEXT("Authored Journal lifecycle and navigation"), [this]()
	{
		BeforeEach([this]()
		{
			Fixture = MakeUnique<FJournalShellFixture>();
			TestNotNull(TEXT("Authored controller loads"), Fixture->Controller);
			TestNotNull(TEXT("Authored BeginPlay creates its existing Journal"), Fixture->Journal);
		});
		AfterEach([this]() { Fixture.Reset(); });

		It(TEXT("should suppress stale empty copy for upstream puzzle observations"), [this]()
		{
			if (!Fixture->Journal) { return; }
			Fixture->Toggle();
			UAtlantisJournalEmptyStateExtension* Extension = Fixture->Journal->GetExtension<UAtlantisJournalEmptyStateExtension>();
			if (!TestNotNull(TEXT("Authored shell empty-state extension"), Extension)
				|| !TestNotNull(TEXT("Current layout accepts empty-state extension"), Extension->EmptyState.Get())) { return; }
			FJournalInvestigationEntry Entry;
			Entry.EntryId = TEXT("NativePuzzleObservation");
			Entry.Title = FText::FromString(TEXT("Visible owner puzzle"));
			Entry.Kind = EJournalEntryKind::PuzzleObservation;
			Entry.Status = EJournalInvestigationStatus::Inprogress;
			Fixture->Instance->GetSubsystem<UAtlantisJournalSubsystem>()->SetInvestigationEntry(Entry);
			UButton* Puzzle = Cast<UButton>(Fixture->Journal->GetWidgetFromName(TEXT("PuzzleTabButton")));
			if (!TestNotNull(TEXT("Authored puzzle navigation"), Puzzle)) { return; }
			Puzzle->OnClicked.Broadcast();
			TestTrue(TEXT("Known upstream puzzle does not show no-puzzles copy"), Extension->EmptyState->GetVisibility() == ESlateVisibility::Collapsed);
		});


		It(TEXT("should open pause and close through the existing Blueprint while restoring gameplay controls"), [this]()
		{
			if (!Fixture->Journal) { return; }
			UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr,
				TEXT("/Game/ProjectAtlantis/Core/Engine/Input/IMC_Default.IMC_Default"));
			Fixture->Controller->AddControls(Context);
			const TArray<UInputMappingContext*> Contexts = Fixture->Controller->GetCurrentMappingContexts();
			TestFalse(TEXT("Gameplay initially unpaused"), Fixture->Instance->GetWorld()->IsPaused());
			TestTrue(TEXT("Actual enhanced Journal input binding executes"), Fixture->Toggle());
			TestTrue(TEXT("Journal added to viewport"), Fixture->Journal->IsInViewport());
			TestTrue(TEXT("Existing game pause engaged"), Fixture->Instance->GetWorld()->IsPaused());
			TestTrue(TEXT("Journal shows cursor"), Fixture->Controller->bShowMouseCursor);
#if UE_ENABLE_DEBUG_DRAWING
			TestEqual(TEXT("Existing UI input mode engaged"), Fixture->Controller->GetCurrentInputModeDebugString(), FString(TEXT("Game and UI")));
#endif
			Fixture->Toggle();
			TestFalse(TEXT("Journal removed from viewport"), Fixture->Journal->IsInViewport());
			TestFalse(TEXT("Gameplay resumed"), Fixture->Instance->GetWorld()->IsPaused());
			TestFalse(TEXT("Gameplay cursor restored"), Fixture->Controller->bShowMouseCursor);
			TestTrue(TEXT("Native gameplay controls remain enabled"), Fixture->Controller->AreControlsEnabled());
			TestTrue(TEXT("Existing desired mapping contexts preserved"), Fixture->Controller->GetCurrentMappingContexts() == Contexts);
			UEnhancedInputLocalPlayerSubsystem* Input = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Fixture->Controller->GetLocalPlayer());
			TestTrue(TEXT("Gameplay mapping is still applied to actual Enhanced Input"), Input && Input->HasMappingContext(Context));
#if UE_ENABLE_DEBUG_DRAWING
			TestTrue(TEXT("Existing Game Only input restored"), Fixture->Controller->GetCurrentInputModeDebugString().StartsWith(TEXT("Game Only")));
#endif
		});

		It(TEXT("should show clear empty states using the existing three Alpha tabs and refresh when investigation entries change"), [this]()
		{
			if (!Fixture->Journal) { return; }
			Fixture->Toggle();
			UAtlantisJournalEmptyStateExtension* Extension = Fixture->Journal->GetExtension<UAtlantisJournalEmptyStateExtension>();
			if (!TestNotNull(TEXT("Controller automatically installs empty-state extension"), Extension)
				|| !TestNotNull(TEXT("Empty-state copy exists in authored shell"), Extension->EmptyState.Get())) { return; }
			UAtlantisJournalSubsystem* Data = Fixture->Instance->GetSubsystem<UAtlantisJournalSubsystem>();
			const TCHAR* Buttons[] = { TEXT("InvestigationTabButton"), TEXT("PuzzleTabButton"), TEXT("DialogueTabButton") };
			for (uint8 Index = 0; Index < 3; ++Index)
			{
				UButton* Button = Cast<UButton>(Fixture->Journal->GetWidgetFromName(Buttons[Index]));
				if (!TestNotNull(Buttons[Index], Button)) { continue; }
				Button->OnClicked.Broadcast();
				const FByteProperty* Tab = FindFProperty<FByteProperty>(Fixture->Journal->GetClass(), TEXT("CurrentTabType"));
				TestEqual(TEXT("Existing Blueprint navigation selects requested section"), Tab->GetPropertyValue_InContainer(Fixture->Journal), Index);
				TestTrue(TEXT("Empty state visible"), Extension->EmptyState->IsVisible());
				TestFalse(TEXT("Empty state contains readable copy"), Extension->EmptyState->GetText().IsEmpty());
			}
			Cast<UButton>(Fixture->Journal->GetWidgetFromName(Buttons[0]))->OnClicked.Broadcast();
			Data->UpdateInvestigationStatus(TEXT("Hidden test entry"), EJournalInvestigationStatus::Undiscovered);
			TestTrue(TEXT("Undiscovered entries preserve clear empty state"), Extension->EmptyState->IsVisible());
			Data->UpdateInvestigationStatus(TEXT("Discovered test entry"), EJournalInvestigationStatus::Inprogress);
			TestTrue(TEXT("Real entry hides Investigation empty state"), Extension->EmptyState->GetVisibility() == ESlateVisibility::Collapsed);
			Data->ClearInvestigations();
			TestTrue(TEXT("Cleared entries restore empty state"), Extension->EmptyState->IsVisible());
			Fixture->Toggle();
			Fixture->Toggle();
			TestNotNull(TEXT("Reopening rebuilds empty-state extension without duplicate bindings"), Extension->EmptyState.Get());
		});
	});

	Describe(TEXT("Authored Objectives tab"), [this]()
	{
		BeforeEach([this]() { Fixture = MakeUnique<FJournalShellFixture>(); });
		AfterEach([this]() { Fixture.Reset(); });
		It(TEXT("should navigate through the authored button and enum while keeping section content exclusive"), [this]()
		{
			if (!TestNotNull(TEXT("Actual Journal shell"), Fixture->Journal)) { return; }
			Fixture->Toggle();
			UWidget* View = Fixture->Journal->GetWidgetFromName(TEXT("ObjectivesTab"));
			UButton* Button = Cast<UButton>(Fixture->Journal->GetWidgetFromName(TEXT("ObjectivesTabButton")));
			if (!TestNotNull(TEXT("Objectives exists in the authored hierarchy"), View) || !TestNotNull(TEXT("Authored button"), Button)) { return; }
			auto* Empty = Fixture->Journal->GetExtension<UAtlantisJournalEmptyStateExtension>();
			if (!TestNotNull(TEXT("Legacy empty-state extension"), Empty)) { return; }
			TestFalse(TEXT("Objectives initially hidden"), View->IsVisible());
			Button->OnClicked.Broadcast();
			const auto* Tab = FindFProperty<FByteProperty>(Fixture->Journal->GetClass(), TEXT("CurrentTabType"));
			TestEqual(TEXT("Objectives uses its own authored enum value"), Tab->GetPropertyValue_InContainer(Fixture->Journal), uint8(4));
			TestEqual(TEXT("Map retains its reserved enum value"), Tab->Enum->GetDisplayNameTextByValue(3).ToString(), FString(TEXT("Map")));
			TestTrue(TEXT("Objectives visible"), View->IsVisible());
			for (const TCHAR* Name : {TEXT("InvestigationTab"), TEXT("PuzzleTab")})
			{
				TestFalse(Name, Fixture->Journal->GetWidgetFromName(Name)->IsVisible());
			}
			TestFalse(TEXT("No overlapping legacy empty copy"), Empty->EmptyState->IsVisible());
			Empty->Destruct(); Empty->Construct();
			TestFalse(TEXT("Late empty-state construction respects selected Objectives"), Empty->EmptyState->IsVisible());
			Fixture->Toggle(); Fixture->Toggle();
			TestTrue(TEXT("Reopening retains authored selection"), View->IsVisible());
			for (const TCHAR* Name : {TEXT("InvestigationTabButton"), TEXT("PuzzleTabButton"), TEXT("DialogueTabButton")})
			{
				CastChecked<UButton>(Fixture->Journal->GetWidgetFromName(Name))->OnClicked.Broadcast();
				TestFalse(TEXT("Existing authored navigation hides Objectives"), View->IsVisible());
				Button->OnClicked.Broadcast();
			}
			CastChecked<UButton>(Fixture->Journal->GetWidgetFromName(TEXT("DialogueTabButton")))->OnClicked.Broadcast();
			TestTrue(TEXT("Dialogue fallback restored"), Empty->EmptyState->IsVisible());
		});
	});

	Describe(TEXT("Merged gamepad Journal bindings"), [this]()
	{
		It(TEXT("should bind the actual Journal action to keyboard and gamepad in land and swimming contexts and allow closing while paused"), [this]()
		{
			UInputAction* Action = LoadObject<UInputAction>(nullptr,
				TEXT("/Game/ProjectAtlantis/Core/Engine/Input/Actions/IA_Journal.IA_Journal"));
			if (!TestNotNull(TEXT("Journal action asset loads"), Action)) { return; }
			TestTrue(TEXT("Journal action triggers while paused"), Action->bTriggerWhenPaused);
			for (const TCHAR* Name : { TEXT("IMC_Default"), TEXT("IMC_Swim") })
			{
				UInputMappingContext* Context = LoadObject<UInputMappingContext>(nullptr,
					*FString::Printf(TEXT("/Game/ProjectAtlantis/Core/Engine/Input/%s.%s"), Name, Name));
				if (!TestNotNull(Name, Context)) { continue; }
				bool bKeyboard = false, bGamepad = false;
				for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
				{
					if (Mapping.Action == Action)
					{
						bKeyboard |= Mapping.Key == EKeys::Tab;
						bGamepad |= Mapping.Key == EKeys::Gamepad_Special_Right;
					}
				}
				TestTrue(FString::Printf(TEXT("%s has Tab binding"), Name), bKeyboard);
				TestTrue(FString::Printf(TEXT("%s has gamepad Special Right binding"), Name), bGamepad);
			}
		});
	});
}
#endif
