// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerAnimInstance.h"
#include "Core/AtlantisPlayerCharacter.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisSwimmingAnimationTest, "Atlantis.Animation.SwimmingTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisSwimmingAnimationTest::RunTest(const FString& Parameters)
{
	TStrongObjectPtr<UWorld> World(UWorld::CreateWorld(EWorldType::Game, false));
	if (!TestNotNull(TEXT("Test world"), World.Get()))
	{
		return false;
	}
	AAtlantisPlayerCharacter* Character = World->SpawnActor<AAtlantisPlayerCharacter>();
	if (!TestNotNull(TEXT("Character"), Character))
	{
		World->DestroyWorld(false);
		return false;
	}
	USkeletalMeshComponent* Mesh = Character->GetMesh();
	Mesh->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/ProjectAtlantis/Characters/Common/Mannequins/SKM_Quinn_Simple.SKM_Quinn_Simple")));
	Mesh->SetAnimInstanceClass(LoadClass<UAnimInstance>(nullptr,
		TEXT("/Game/ProjectAtlantis/Characters/Common/Animations/Swimming/ABP_Player.ABP_Player_C")));
	UAtlantisPlayerAnimInstance* Anim = Cast<UAtlantisPlayerAnimInstance>(Mesh->GetAnimInstance());
	if (!TestNotNull(TEXT("Swimming animation Blueprint uses native player state"), Anim))
	{
		World->DestroyWorld(false);
		return false;
	}
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	Movement->bRunPhysicsWithNoController = true;
	auto Update = [Movement, Anim](EMovementMode Mode, FVector Velocity, FVector Input)
	{
		Movement->SetMovementMode(Mode);
		Movement->Velocity = Velocity;
		Movement->AddInputVector(Input);
		// Zero elapsed time updates input acceleration without advancing physics outside a water volume.
		Movement->TickComponent(0.f, LEVELTICK_All, nullptr);
		Anim->NativeUpdateAnimation(1.f / 30.f);
	};
	Update(MOVE_Walking, FVector(100.f, 0.f, 0.f), FVector::ForwardVector);
	TestFalse(TEXT("Land movement selects original animation graph"), Anim->bIsSwimming);
	Update(MOVE_Swimming, FVector::ZeroVector, FVector::ZeroVector);
	TestTrue(TEXT("Water entry selects swimming"), Anim->bIsSwimming);
	TestFalse(TEXT("Stationary underwater uses idle"), Anim->bIsSwimmingMoving);
	Update(MOVE_Swimming, FVector(0.f, 0.f, -80.f), FVector::ZeroVector);
	TestFalse(TEXT("Passive ballast descent uses idle"), Anim->bIsSwimmingMoving);
	Update(MOVE_Swimming, FVector(0.f, 0.f, 80.f), FVector::ZeroVector);
	TestFalse(TEXT("Passive ballast ascent uses idle"), Anim->bIsSwimmingMoving);
	Update(MOVE_Swimming, FVector(100.f, 0.f, 0.f), FVector::ForwardVector);
	TestTrue(TEXT("Active forward swim uses forward loop"), Anim->bIsSwimmingMoving);
	Update(MOVE_Swimming, FVector(0.f, 0.f, 100.f), FVector::UpVector);
	TestTrue(TEXT("Active vertical swim uses forward loop"), Anim->bIsSwimmingMoving);
	Update(MOVE_Swimming, FVector(100.f, 0.f, 0.f), FVector::ZeroVector);
	TestFalse(TEXT("Releasing movement returns to idle despite residual velocity"), Anim->bIsSwimmingMoving);
	Update(MOVE_Falling, FVector(0.f, 0.f, -100.f), FVector::ZeroVector);
	TestFalse(TEXT("Water exit in air restores original graph"), Anim->bIsSwimming);
	TestFalse(TEXT("Water exit clears moving swim state"), Anim->bIsSwimmingMoving);
	Update(MOVE_Walking, FVector::ZeroVector, FVector::ZeroVector);
	TestFalse(TEXT("Walking after exit stays on original graph"), Anim->bIsSwimming);
	Update(MOVE_Swimming, FVector::ZeroVector, FVector::ZeroVector);
	TestTrue(TEXT("Re-entering water selects swimming again"), Anim->bIsSwimming);
	auto EvaluatePose = [Mesh]()
	{
		for (int32 Frame = 0; Frame < 15; ++Frame)
		{
			Mesh->TickAnimation(1.f / 30.f, false);
			Mesh->RefreshBoneTransforms();
		}
		return Mesh->GetSocketTransform(TEXT("head"), RTS_Component).GetLocation();
	};
	const FVector IdleHead = EvaluatePose();
	Update(MOVE_Swimming, FVector(100.f, 0.f, 0.f), FVector::ForwardVector);
	const FVector SwimmingHead = EvaluatePose();
	TestTrue(TEXT("Animation graph evaluates a distinct forward swimming pose"),
		FMath::Abs(SwimmingHead.Y - IdleHead.Y) > 25.f);
	Update(MOVE_Walking, FVector::ZeroVector, FVector::ZeroVector);
	const FVector LandHead = EvaluatePose();
	TestTrue(TEXT("Leaving water evaluates the upright original land pose"), LandHead.Z > SwimmingHead.Z + 20.f);
	World->DestroyWorld(false);
	return true;
}

#endif
