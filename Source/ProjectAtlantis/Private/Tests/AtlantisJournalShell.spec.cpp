// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS

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
			const UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Controller ? Controller->InputComponent : nullptr);
			if (!Input)
			{
				return false;
			}

			for (const TUniquePtr<FEnhancedInputActionEventBinding>& Binding : Input->GetActionEventBindings())
			{
				const UInputAction* Action = Binding->GetAction();
				if (!Action || Action->GetFName() != TEXT("IA_Journal") || Binding->GetTriggerEvent() != ETriggerEvent::Started)
				{
					continue;
				}

				Binding->Execute(FInputActionInstance(Action));
				return true;
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

		It(TEXT("should display each authored empty state and refresh disclosed clue and puzzle content"), [this]()
		{
			if (!Fixture->Journal) { return; }
			Fixture->Toggle();
			UAtlantisJournalSubsystem* Data = Fixture->Instance->GetSubsystem<UAtlantisJournalSubsystem>();
			const TCHAR* Buttons[] = {TEXT("InvestigationTabButton"), TEXT("PuzzleTabButton")};
			const TCHAR* Views[] = {TEXT("InvestigationTab"), TEXT("PuzzleTab")};
			for (uint8 Index = 0; Index < 2; ++Index)
			{
				UButton* Button = Cast<UButton>(Fixture->Journal->GetWidgetFromName(Buttons[Index]));
				UUserWidget* View = Cast<UUserWidget>(Fixture->Journal->GetWidgetFromName(Views[Index]));
				if (!TestNotNull(Buttons[Index], Button) || !TestNotNull(Views[Index], View)) { continue; }
				Button->OnClicked.Broadcast();
				UTextBlock* Empty = Cast<UTextBlock>(View->GetWidgetFromName(TEXT("EmptyState")));
				if (!TestNotNull(TEXT("Authored section owns its empty copy"), Empty)) { continue; }
				TestTrue(TEXT("Empty section has readable visible copy"), Empty->IsVisible() && !Empty->GetText().IsEmptyOrWhitespace());
				FJournalInvestigationEntry Entry;
				Entry.EntryId = TEXT("OwnerEntry");
				Entry.Title = FText::FromString(TEXT("Owner supplied entry"));
				Entry.Kind = Index == 0 ? EJournalEntryKind::Clue : EJournalEntryKind::PuzzleObservation;
				Data->SetInvestigationEntry(Entry);
				TestTrue(TEXT("Undisclosed data keeps the section empty"), Empty->IsVisible());
				Entry.Status = EJournalInvestigationStatus::Inprogress;
				Data->SetInvestigationEntry(Entry);
				TestFalse(TEXT("Disclosed data hides authored fallback"), Empty->IsVisible());
				Data->ClearInvestigations();
				TestTrue(TEXT("Clear restores authored fallback"), Empty->IsVisible());
			}
			CastChecked<UButton>(Fixture->Journal->GetWidgetFromName(TEXT("DialogueTabButton")))->OnClicked.Broadcast();
			UUserWidget* Dialogue = Cast<UUserWidget>(Fixture->Journal->GetWidgetFromName(TEXT("DialogueTab")));
			if (!TestNotNull(TEXT("Dialogue is an authored UserWidget tab"), Dialogue)) { return; }
			TestEqual(TEXT("Dialogue tab asset is a separate Blueprint"), Dialogue->GetClass()->GetPathName(), FString(TEXT("/Game/ProjectAtlantis/UI/WBP_JournalDialogueTab.WBP_JournalDialogueTab_C")));
			UTextBlock* DialogueEmpty = Cast<UTextBlock>(Dialogue->GetWidgetFromName(TEXT("EmptyState")));
			TestNotNull(TEXT("Dialogue fallback is authored in its section"), DialogueEmpty);
			TestTrue(TEXT("Dialogue section is visible"), Fixture->Journal->GetWidgetFromName(TEXT("DialogueTab"))->IsVisible());
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
			TestFalse(TEXT("Objectives initially hidden"), View->IsVisible());
			Button->OnClicked.Broadcast();
			const auto* Tab = FindFProperty<FByteProperty>(Fixture->Journal->GetClass(), TEXT("CurrentTabType"));
			TestEqual(TEXT("Objectives uses its own authored enum value"), Tab->GetPropertyValue_InContainer(Fixture->Journal), uint8(4));
			TestEqual(TEXT("Map retains its reserved enum value"), Tab->Enum->GetDisplayNameTextByValue(3).ToString(), FString(TEXT("Map")));
			TestTrue(TEXT("Objectives visible"), View->IsVisible());
			for (const TCHAR* Name : {TEXT("InvestigationTab"), TEXT("PuzzleTab"), TEXT("DialogueTab")})
			{
				TestFalse(Name, Fixture->Journal->GetWidgetFromName(Name)->IsVisible());
			}
			Fixture->Toggle(); Fixture->Toggle();
			TestTrue(TEXT("Reopening retains authored selection"), View->IsVisible());
			for (const TCHAR* Name : {TEXT("InvestigationTabButton"), TEXT("PuzzleTabButton"), TEXT("DialogueTabButton")})
			{
				CastChecked<UButton>(Fixture->Journal->GetWidgetFromName(Name))->OnClicked.Broadcast();
				TestFalse(TEXT("Existing authored navigation hides Objectives"), View->IsVisible());
				Button->OnClicked.Broadcast();
			}
			CastChecked<UButton>(Fixture->Journal->GetWidgetFromName(TEXT("DialogueTabButton")))->OnClicked.Broadcast();
			TestTrue(TEXT("Authored Dialogue fallback restored"), Fixture->Journal->GetWidgetFromName(TEXT("DialogueTab"))->IsVisible());
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
				bool bKeyboard = false;
				bool bGamepad = false;
				for (const FEnhancedActionKeyMapping& Mapping : Context->GetMappings())
				{
					if (Mapping.Action != Action)
					{
						continue;
					}
					bKeyboard |= Mapping.Key == EKeys::Tab;
					bGamepad |= Mapping.Key == EKeys::Gamepad_Special_Right;
				}
				TestTrue(FString::Printf(TEXT("%s has Tab binding"), Name), bKeyboard);
				TestTrue(FString::Printf(TEXT("%s has gamepad Special Right binding"), Name), bGamepad);
			}
		});
	});
}
#endif
