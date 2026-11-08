// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "Core/AtlantisAirPocketVolume.h"
#include "Core/AtlantisWaterVolume.h"
#include "Core/AtlantisPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

BEGIN_DEFINE_SPEC(FAtlantisAirPocketSpec, "Atlantis.Traversal.AirPocket",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TStrongObjectPtr<UWorld> World;
END_DEFINE_SPEC(FAtlantisAirPocketSpec)

void FAtlantisAirPocketSpec::Define()
{
	BeforeEach([this]()
	{
		const UWorld::InitializationValues Values = UWorld::InitializationValues()
			.AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
		World.Reset(UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true,
			ERHIFeatureLevel::Num, &Values));
		GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World.Get());
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
	});
	AfterEach([this]()
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World.Get());
		World.Reset();
	});
	Describe("nested dry volume", [this]()
	{
		It("should require the character center inside the higher priority dry boundary", [this]()
		{
			AAtlantisWaterVolume* Water = World->SpawnActor<AAtlantisWaterVolume>();
			AAtlantisAirPocketVolume* Air = World->SpawnActor<AAtlantisAirPocketVolume>();
			AAtlantisPlayerCharacter* Character = World->SpawnActor<AAtlantisPlayerCharacter>();
			Character->GetCharacterMovement()->GetNavAgentPropertiesRef().bCanSwim = true;
			Character->GetCapsuleComponent()->SetShouldUpdatePhysicsVolume(true);
			Character->SetActorLocation(FVector(2000.f, 0.f, 0.f));
			TestTrue(TEXT("Water contains the route"), Water->IsOverlapInVolume(*Character->GetCapsuleComponent()));
			TestFalse(TEXT("Bubble excludes the swimming route"), Air->IsOverlapInVolume(*Character->GetCapsuleComponent()));
			Character->SetActorLocation(FVector::ZeroVector);
			TestTrue(TEXT("Bubble contains its center"), Air->IsOverlapInVolume(*Character->GetCapsuleComponent()));
			TestTrue(TEXT("Dry volume wins over overlapping water"), Air->Priority > Water->Priority);
			TestFalse(TEXT("Bubble is breathable"), Air->bWaterVolume);
			Character->SetActorLocation(FVector(Air->AirBoundary->GetScaledSphereRadius() + 10.f, 0.f, 0.f));
			TestFalse(TEXT("A capsule touching air from outside remains underwater"), Air->IsOverlapInVolume(*Character->GetCapsuleComponent()));
		});
	});
}
#endif
