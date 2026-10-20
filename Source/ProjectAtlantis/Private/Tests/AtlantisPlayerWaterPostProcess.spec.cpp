// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Core/AtlantisPlayerWaterPostProcess.h"
#include "Builders/CubeBuilder.h"
#include "ActorFactories/ActorFactory.h"
#include "CommonGameViewportClient.h"
#include "Components/SceneComponent.h"
#include "Components/BrushComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "SceneView.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(FAtlantisPlayerWaterPostProcessSpec, "Atlantis.Rendering.PlayerWater",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UGameInstance> Instance;
	TStrongObjectPtr<UCommonGameViewportClient> Viewport;
	AAtlantisPlayerWaterPostProcess* Gate = nullptr;
	APostProcessVolume* Source = nullptr;
	APawn* Pawns[2] = {};
	ULocalPlayer* Players[2] = {};
	APhysicsVolume* Water = nullptr;
END_DEFINE_SPEC(FAtlantisPlayerWaterPostProcessSpec)

void FAtlantisPlayerWaterPostProcessSpec::Define()
{
	BeforeEach([this]()
	{
		Instance.Reset(NewObject<UGameInstance>(GEngine));
		Instance->InitializeStandalone();
		Viewport.Reset(NewObject<UCommonGameViewportClient>(GEngine));
		Instance->GetWorldContext()->GameViewport = Viewport.Get();
		Viewport->Init(*Instance->GetWorldContext(), Instance.Get(), false);
		UWorld* World = Instance->GetWorld();
		Gate = World->SpawnActor<AAtlantisPlayerWaterPostProcess>();
		Source = World->SpawnActor<APostProcessVolume>();
		UCubeBuilder* Builder = NewObject<UCubeBuilder>();
		Builder->X = Builder->Y = Builder->Z = 1000.f;
		// SpawnActor alone has no UModel. Use the editor's real volume creation path,
		// which initializes the brush model/polygons before building collision.
		UActorFactory::CreateBrushForVolumeActor(Source, Builder);
		TestNotNull(TEXT("Real source brush built"), Source->GetBrushComponent()->Brush.Get());
		Source->BlendRadius = 0.f;
		Source->BlendWeight = 1.f;
		Gate->SourceVolume = Source;
		Water = World->SpawnActor<APhysicsVolume>();
		Water->bWaterVolume = true;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			FString Error;
			Players[Index] = Instance->CreateLocalPlayer(Index, Error, false);
			TestNotNull(TEXT("Local player created"), Players[Index]);
			APlayerController* Controller = World->SpawnActor<APlayerController>();
			Controller->SetPlayer(Players[Index]);
			World->AddController(Controller);
			Pawns[Index] = World->SpawnActor<APawn>();
			USceneComponent* Root = NewObject<USceneComponent>(Pawns[Index]);
			Pawns[Index]->SetRootComponent(Root);
			Root->RegisterComponent();
			Controller->Possess(Pawns[Index]);
		}
		Pawns[0]->GetRootComponent()->SetPhysicsVolume(Water, false);
	});
	AfterEach([this]()
	{
		UWorld* World = Instance->GetWorld();
		Instance->Shutdown();
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		Viewport.Reset(); Instance.Reset(); Gate = nullptr; Source = nullptr; Water = nullptr;
	});
	Describe(TEXT("Player-owned water inclusion"), [this]()
	{
		It(TEXT("should gate on actual pawn water state and source brush rather than camera position"), [this]()
		{
			TestEqual(TEXT("Wet player inside original region"), Gate->GetPlayerBlendWeight(Pawns[0]), 1.f);
			TestEqual(TEXT("Dry player at same point has no water effect"), Gate->GetPlayerBlendWeight(Pawns[1]), 0.f);
			Pawns[0]->SetActorLocation(FVector(5000.f, 0.f, 0.f));
			TestEqual(TEXT("Different water region excluded"), Gate->GetPlayerBlendWeight(Pawns[0]), 0.f);
			Pawns[0]->SetActorLocation(FVector::ZeroVector);
			Source->bEnabled = false;
			TestEqual(TEXT("Original enabled state honored"), Gate->GetPlayerBlendWeight(Pawns[0]), 0.f);
		});
		It(TEXT("should resolve each local view independently including camera targets outside the pawn"), [this]()
		{
			FSceneViewInitOptions Options;
			Options.SetViewRectangle(FIntRect(0, 0, 128, 128));
			Options.ViewOrigin = FVector(9000.f, 0.f, 0.f);
			Options.ViewActor = Source;
			Options.PlayerIndex = Players[0]->GetControllerId();
			FSceneView First(Options);
			TestTrue(TEXT("Owning wet pawn resolved despite camera target"), Gate->ResolveViewPawn(First) == Pawns[0]);
			TestEqual(TEXT("Wet player retains effect with camera outside water"), Gate->GetPlayerBlendWeight(Gate->ResolveViewPawn(First)), 1.f);
			Options.ViewOrigin = FVector::ZeroVector;
			Options.PlayerIndex = Players[1]->GetControllerId();
			FSceneView Second(Options);
			TestTrue(TEXT("Second view gets its dry pawn"), Gate->ResolveViewPawn(Second) == Pawns[1]);
			TestEqual(TEXT("Second view remains dry"), Gate->GetPlayerBlendWeight(Gate->ResolveViewPawn(Second)), 0.f);
			Options.PlayerIndex = INDEX_NONE;
			FSceneView Capture(Options);
			TestNull(TEXT("Non-player captures are excluded"), Gate->ResolveViewPawn(Capture));
		});
		It(TEXT("should preserve source weights and fail closed when source geometry or player is missing"), [this]()
		{
			Source->BlendWeight = 0.5f;
			Gate->MaterialWeight = 0.4f;
			TestEqual(TEXT("Authored weights retained"), Gate->GetPlayerBlendWeight(Pawns[0]), 0.2f);
			Gate->SourceVolume = Instance->GetWorld()->SpawnActor<APostProcessVolume>();
			TestEqual(TEXT("Missing source collision excluded"), Gate->GetPlayerBlendWeight(Pawns[0]), 0.f);
			Gate->SourceVolume = nullptr;
			TestEqual(TEXT("Missing source excluded"), Gate->GetPlayerBlendWeight(Pawns[0]), 0.f);
			TestEqual(TEXT("Missing player excluded"), Gate->GetPlayerBlendWeight(nullptr), 0.f);
		});
	});
}
#endif
