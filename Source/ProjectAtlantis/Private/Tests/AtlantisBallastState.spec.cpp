// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerState.h"
#include "Tests/AtlantisBallastTestListener.h"
#include "Engine/World.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/AutomationTest.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"

namespace
{
	// Each test owns a minimal runtime world, independent of the editor's current map.
	struct FAtlantisBallastFixture
	{
		TStrongObjectPtr<UWorld> World;
		AAtlantisPlayerState* State = nullptr;
		TStrongObjectPtr<UAtlantisBallastTestListener> Listener;

		FAtlantisBallastFixture()
			: World(UWorld::CreateWorld(EWorldType::Game, false)),
			  Listener(NewObject<UAtlantisBallastTestListener>())
		{
			if (!World.IsValid())
			{
				return;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			State = World->SpawnActor<AAtlantisPlayerState>(SpawnParameters);
			if (State)
			{
				State->OnBallastChanged.AddDynamic(Listener.Get(), &UAtlantisBallastTestListener::RecordBallastChanged);
				State->OnLockedOxygenChanged.AddDynamic(Listener.Get(), &UAtlantisBallastTestListener::RecordLockedOxygenChanged);
			}
		}

		~FAtlantisBallastFixture()
		{
			if (World.IsValid())
			{
				World->DestroyWorld(false);
			}
		}
	};
}

BEGIN_DEFINE_SPEC(FAtlantisBallastStateSpec, "Atlantis.PlayerState.BallastAllocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TUniquePtr<FAtlantisBallastFixture> Fixture;
END_DEFINE_SPEC(FAtlantisBallastStateSpec)

void FAtlantisBallastStateSpec::Define()
{
	BeforeEach([this]()
	{
		Fixture = MakeUnique<FAtlantisBallastFixture>();
		TestNotNull(TEXT("Test world created"), Fixture->World.Get());
		TestNotNull(TEXT("Player state spawned"), Fixture->State);
	});

	AfterEach([this]()
	{
		Fixture.Reset();
	});

	Describe(TEXT("InitialState"), [this]()
	{
		It(TEXT("should start in Descend without a reservation"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			TestTrue(TEXT("Initial mode is Descend"), Fixture->State->GetBallastState() == EAtlantisBallastState::Descend);
			TestEqual(TEXT("Descend reserves no oxygen"), Fixture->State->GetLockedOxygen(), 0.f);
			TestFalse(TEXT("Initial mode is not allocated"), Fixture->State->IsBallastAllocated());
		});
	});

	Describe(TEXT("RejectedAllocation"), [this]()
	{
		It(TEXT("should reject requests without oxygen headroom and emit no events"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			for (const float Oxygen : {0.5f, 1.f})
			{
				Fixture->State->SetCurrentOxygen(Oxygen);
				Fixture->State->SetBallastState(EAtlantisBallastState::Wander);
				TestTrue(TEXT("Insufficient or exact allocation is rejected"), Fixture->State->GetBallastState() == EAtlantisBallastState::Descend);
				TestEqual(TEXT("Rejected mode preserves reservation"), Fixture->State->GetLockedOxygen(), 0.f);
				TestEqual(TEXT("Rejected mode preserves total oxygen"), Fixture->State->GetCurrentOxygen(), Oxygen);
				TestEqual(TEXT("Rejected mode emits no ballast event"), Fixture->Listener->BallastEventCount, 0);
				TestEqual(TEXT("Rejected mode emits no reservation event"), Fixture->Listener->LockedOxygenEventCount, 0);
			}
		});
	});

	Describe(TEXT("ModeTransitions"), [this]()
	{
		It(TEXT("should replace reservations and emit one event with the new mode"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			Fixture->State->SetCurrentOxygen(3.f);
			Fixture->State->SetBallastState(EAtlantisBallastState::Wander);
			TestEqual(TEXT("Wander reservation"), Fixture->State->GetLockedOxygen(), 1.f);
			TestTrue(TEXT("Wander is allocated"), Fixture->State->IsBallastAllocated());
			TestEqual(TEXT("Allocation preserves total oxygen"), Fixture->State->GetCurrentOxygen(), 3.f);
			TestEqual(TEXT("Wander emits exactly one ballast event"), Fixture->Listener->BallastEventCount, 1);
			TestTrue(TEXT("Wander event reports allocated"), Fixture->Listener->bLastAllocated);
			TestTrue(TEXT("Wander event reports mode"), Fixture->Listener->LastBallastState == EAtlantisBallastState::Wander);
			Fixture->Listener->Reset();
			Fixture->State->SetBallastState(EAtlantisBallastState::Ascend);
			TestEqual(TEXT("Ascend replaces reservation"), Fixture->State->GetLockedOxygen(), 2.f);
			TestEqual(TEXT("Ascend preserves total oxygen"), Fixture->State->GetCurrentOxygen(), 3.f);
			TestEqual(TEXT("Ascend emits exactly one ballast event"), Fixture->Listener->BallastEventCount, 1);
			TestTrue(TEXT("Ascend event reports allocated"), Fixture->Listener->bLastAllocated);
			TestTrue(TEXT("Ascend event reports mode"), Fixture->Listener->LastBallastState == EAtlantisBallastState::Ascend);
		});
	});

	Describe(TEXT("RepeatedMode"), [this]()
	{
		It(TEXT("should preserve state and emit no events for the current mode"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			Fixture->State->SetBallastState(EAtlantisBallastState::Descend);
			TestEqual(TEXT("Repeating initial mode emits no ballast event"), Fixture->Listener->BallastEventCount, 0);
			TestEqual(TEXT("Repeating initial mode emits no reservation event"), Fixture->Listener->LockedOxygenEventCount, 0);
			Fixture->State->SetBallastState(EAtlantisBallastState::Wander);
			Fixture->Listener->Reset();
			Fixture->State->SetBallastState(EAtlantisBallastState::Wander);
			TestTrue(TEXT("Repeating Wander preserves mode"), Fixture->State->GetBallastState() == EAtlantisBallastState::Wander);
			TestEqual(TEXT("Repeating Wander preserves reservation"), Fixture->State->GetLockedOxygen(), 1.f);
			TestEqual(TEXT("Repeating Wander emits no ballast event"), Fixture->Listener->BallastEventCount, 0);
			TestEqual(TEXT("Repeating Wander emits no reservation event"), Fixture->Listener->LockedOxygenEventCount, 0);
		});
	});

	Describe(TEXT("ConsumptionProtection"), [this]()
	{
		It(TEXT("should protect reserved oxygen from consumption"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			Fixture->State->SetBallastState(EAtlantisBallastState::Ascend);
			Fixture->State->SetCurrentOxygen(0.f);
			TestEqual(TEXT("Consumption preserves reserved oxygen"), Fixture->State->GetCurrentOxygen(), 2.f);
			TestEqual(TEXT("Reservation remains unchanged"), Fixture->State->GetLockedOxygen(), 2.f);
			TestEqual(TEXT("Reserved oxygen is unavailable for consumption"), Fixture->State->GetAvailableOxygen(), 0.f);
		});
	});

	Describe(TEXT("ReservationRelease"), [this]()
	{
		It(TEXT("should release oxygen and emit one unallocated event on Descend"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			Fixture->State->SetBallastState(EAtlantisBallastState::Ascend);
			Fixture->Listener->Reset();
			Fixture->State->SetBallastState(EAtlantisBallastState::Descend);
			TestEqual(TEXT("Descend releases reservation"), Fixture->State->GetLockedOxygen(), 0.f);
			TestFalse(TEXT("Descend is not allocated"), Fixture->State->IsBallastAllocated());
			TestEqual(TEXT("Descend emits exactly one ballast event"), Fixture->Listener->BallastEventCount, 1);
			TestFalse(TEXT("Descend event reports unallocated"), Fixture->Listener->bLastAllocated);
			TestTrue(TEXT("Descend event reports mode"), Fixture->Listener->LastBallastState == EAtlantisBallastState::Descend);
			Fixture->State->SetCurrentOxygen(0.f);
			TestEqual(TEXT("Released oxygen can be consumed"), Fixture->State->GetCurrentOxygen(), 0.f);
		});
	});

	Describe(TEXT("DirectReservationEvents"), [this]()
	{
		It(TEXT("should derive allocation and emit only oxygen events for direct changes"), [this]()
		{
			if (!Fixture->State)
			{
				return;
			}
			Fixture->State->SetLockedOxygen(1.f);
			TestTrue(TEXT("Direct reservation sets derived allocation"), Fixture->State->IsBallastAllocated());
			TestEqual(TEXT("Direct reservation emits one oxygen event"), Fixture->Listener->LockedOxygenEventCount, 1);
			TestEqual(TEXT("Direct reservation reports old oxygen"), Fixture->Listener->LastOldLockedOxygen, 0.f);
			TestEqual(TEXT("Direct reservation reports new oxygen"), Fixture->Listener->LastNewLockedOxygen, 1.f);
			TestEqual(TEXT("Direct reservation emits no ballast event"), Fixture->Listener->BallastEventCount, 0);
			Fixture->Listener->Reset();
			Fixture->State->SetLockedOxygen(0.f);
			TestFalse(TEXT("Direct release clears derived allocation"), Fixture->State->IsBallastAllocated());
			TestEqual(TEXT("Direct release emits one oxygen event"), Fixture->Listener->LockedOxygenEventCount, 1);
			TestEqual(TEXT("Direct release reports old oxygen"), Fixture->Listener->LastOldLockedOxygen, 1.f);
			TestEqual(TEXT("Direct release reports new oxygen"), Fixture->Listener->LastNewLockedOxygen, 0.f);
			TestEqual(TEXT("Direct release emits no ballast event"), Fixture->Listener->BallastEventCount, 0);
		});
	});

}

namespace
{
	// Run the fatal case in another editor process.
	class FAtlantisBallastFatalCommand : public IAutomationLatentCommand
	{
	public:
		FAtlantisBallastFatalCommand(FAutomationTestBase* InTest, FProcHandle InProcess, const FString& InChildLogFilePath)
			: Test(InTest), Process(InProcess), ChildLogFilePath(InChildLogFilePath), StartTime(FPlatformTime::Seconds())
		{
		}

		virtual ~FAtlantisBallastFatalCommand() override
		{
			if (Process.IsValid())
			{
				if (FPlatformProcess::IsProcRunning(Process))
				{
					FPlatformProcess::TerminateProc(Process, true);
				}
				FPlatformProcess::CloseProc(Process);
			}
		}

		virtual bool Update() override
		{
			if (FPlatformProcess::IsProcRunning(Process))
			{
				if (FPlatformTime::Seconds() - StartTime < 180.0)
				{
					return false;
				}
				Test->AddError(FString::Printf(TEXT("Fatal ballast child test timed out. See %s"), *ChildLogFilePath));
				return true;
			}

			int32 ReturnCode = 0;
			const bool bHasReturnCode = FPlatformProcess::GetProcReturnCode(Process, &ReturnCode);
			Test->TestTrue(TEXT("Invalid ballast request terminates the child process"), bHasReturnCode && ReturnCode != 0);
			FString Log;
			Test->TestTrue(TEXT("Child process log is available"), FFileHelper::LoadFileToString(Log, *ChildLogFilePath));
			Test->TestTrue(TEXT("Child reached the invalid ballast request"), Log.Contains(TEXT("AtlantisBallastFatalProbe: requesting None")));
			Test->TestTrue(TEXT("Allocation helper reported the expected fatal error"), Log.Contains(TEXT("Fatal error:")) && Log.Contains(TEXT("Invalid ballast state: 0")));
			Test->TestFalse(TEXT("Invalid request never returns"), Log.Contains(TEXT("AtlantisBallastFatalProbe: request returned")));
			return true;
		}

	private:
		FAutomationTestBase* Test;
		FProcHandle Process;
		FString ChildLogFilePath;
		double StartTime;
	};

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAtlantisBallastInvalidStateFatalTest, "Atlantis.PlayerState.BallastAllocation.InvalidStateFatal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAtlantisBallastInvalidStateFatalTest::RunTest(const FString& Parameters)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("AtlantisBallastFatalProbe")))
	{
		const FAtlantisBallastFixture Fixture;
		if (!TestNotNull(TEXT("Player state spawned"), Fixture.State))
		{
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("AtlantisBallastFatalProbe: requesting None"));
		Fixture.State->SetBallastState(EAtlantisBallastState::None);
		UE_LOG(LogTemp, Error, TEXT("AtlantisBallastFatalProbe: request returned"));
		return false;
	}

	const FString ChildLogFilePath = FPaths::ConvertRelativePathToFull(FPaths::CreateTempFilename(*FPaths::ProjectLogDir(), TEXT("BallastFatal"), TEXT(".log")));
	const FString Arguments = FString::Printf(
		TEXT("\"%s\" -unattended -nop4 -NullRHI -nosplash -NoCrashDialog -AtlantisBallastFatalProbe -abslog=\"%s\" -ExecCmds=\"Automation RunTests Atlantis.PlayerState.BallastAllocation.InvalidStateFatal\" -TestExit=\"Automation Test Queue Empty\""),
		*FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()), *ChildLogFilePath);
	FProcHandle Process = FPlatformProcess::CreateProc(FPlatformProcess::ExecutablePath(), *Arguments, false, true, true, nullptr, 0, nullptr, nullptr);
	if (!TestTrue(TEXT("Fatal ballast child process started"), Process.IsValid()))
	{
		return false;
	}
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FAtlantisBallastFatalCommand>(this, Process, ChildLogFilePath));
	return true;
}

#endif
