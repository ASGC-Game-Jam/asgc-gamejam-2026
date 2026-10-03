// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerState.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisBallastAllocationTest, "Atlantis.PlayerState.BallastAllocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisBallastAllocationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	AAtlantisPlayerState* State = World->SpawnActor<AAtlantisPlayerState>();
	if (!TestNotNull(TEXT("Player state spawned"), State))
	{
		World->DestroyWorld(false);
		return false;
	}

	State->DispatchBeginPlay();
	TestTrue(TEXT("Initial mode is Descend"), State->GetBallastState() == EAtlantisBallastState::Descend);
	TestEqual(TEXT("Descend reserves no oxygen"), State->GetLockedOxygen(), 0.f);
	State->SetCurrentOxygen(1.f);
	State->SetBallastState(EAtlantisBallastState::Wander);
	TestTrue(TEXT("Exact allocation is rejected"), State->GetBallastState() == EAtlantisBallastState::Descend);
	State->SetCurrentOxygen(3.f);
	State->SetBallastState(EAtlantisBallastState::Wander);
	TestEqual(TEXT("Wander reservation"), State->GetLockedOxygen(), 1.f);
	TestTrue(TEXT("Wander is allocated"), State->IsBallastAllocated());
	TestEqual(TEXT("Allocation preserves total oxygen"), State->GetCurrentOxygen(), 3.f);
	State->SetBallastState(EAtlantisBallastState::Ascend);
	TestEqual(TEXT("Ascend replaces reservation"), State->GetLockedOxygen(), 2.f);
	State->SetCurrentOxygen(0.f);
	TestEqual(TEXT("Consumption preserves reserved oxygen"), State->GetCurrentOxygen(), 2.f);
	AddExpectedError(TEXT("Invalid ballast state: 0"), EAutomationExpectedErrorFlags::Contains, 1);
	State->SetBallastState(EAtlantisBallastState::None);
	TestTrue(TEXT("None cannot replace a valid mode"), State->GetBallastState() == EAtlantisBallastState::Ascend);
	TestEqual(TEXT("Invalid request preserves reservation"), State->GetLockedOxygen(), 2.f);
	State->SetBallastState(EAtlantisBallastState::Descend);
	TestEqual(TEXT("Descend releases reservation"), State->GetLockedOxygen(), 0.f);
	TestFalse(TEXT("Descend clears allocation flag"), State->IsBallastAllocated());
	State->SetCurrentOxygen(0.f);
	TestEqual(TEXT("Released oxygen can be consumed"), State->GetCurrentOxygen(), 0.f);

	World->DestroyWorld(false);
	return true;
}

#endif
