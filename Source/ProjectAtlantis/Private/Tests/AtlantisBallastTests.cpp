// Copyright (c) 2026 ASGC

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/AtlantisPlayerState.h"
#include "Engine/World.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"

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
	if (FParse::Param(FCommandLine::Get(), TEXT("AtlantisBallastFatalProbe")))
	{
		UE_LOG(LogTemp, Display, TEXT("AtlantisBallastFatalProbe: requesting None"));
		State->SetBallastState(EAtlantisBallastState::None);
		UE_LOG(LogTemp, Error, TEXT("AtlantisBallastFatalProbe: request returned"));
		World->DestroyWorld(false);
		return false;
	}
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
	State->SetBallastState(EAtlantisBallastState::Descend);
	TestEqual(TEXT("Descend releases reservation"), State->GetLockedOxygen(), 0.f);
	TestFalse(TEXT("Descend clears allocation flag"), State->IsBallastAllocated());
	State->SetCurrentOxygen(0.f);
	TestEqual(TEXT("Released oxygen can be consumed"), State->GetCurrentOxygen(), 0.f);

	World->DestroyWorld(false);

	const FString ChildLogFilePath = FPaths::ConvertRelativePathToFull(FPaths::CreateTempFilename(*FPaths::ProjectLogDir(), TEXT("BallastFatal"), TEXT(".log")));
	const FString Arguments = FString::Printf(
		TEXT("\"%s\" -unattended -nop4 -NullRHI -nosplash -NoCrashDialog -AtlantisBallastFatalProbe -abslog=\"%s\" -ExecCmds=\"Automation RunTests Atlantis.PlayerState.BallastAllocation\" -TestExit=\"Automation Test Queue Empty\""),
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
