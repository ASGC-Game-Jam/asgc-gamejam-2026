// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "GitSourceControlUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

BEGIN_DEFINE_SPEC(FGitStatusParsingSpec, "Atlantis.Git.StatusParsing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FGitStatusParsingSpec)

void FGitStatusParsingSpec::Define()
{
	It("should report unstaged work when both porcelain columns are changed", [this]()
	{
		struct FCase { const TCHAR* Status; EFileState::Type FileState; };
		const FCase Cases[] = {{TEXT("MM"), EFileState::Modified}, {TEXT("AM"), EFileState::Added}, {TEXT("MD"), EFileState::Missing}};
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		const FString File = Root / TEXT("Content/StatusFixture.uasset");
		for (const FCase& Case : Cases)
		{
			TMap<FString, FString> Results;
			Results.Add(File, FString(Case.Status) + TEXT(" Content/StatusFixture.uasset"));
			TMap<FString, FGitSourceControlState> States;
			GitSourceControlUtils::ParseStatusResults(TEXT(""), Root, false, {File}, Results, States);
			const FGitSourceControlState* State = States.Find(File);
			TestNotNull(Case.Status, State);
			if (State)
			{
				TestEqual(TEXT("Worktree state is deterministic"), State->State.TreeState, ETreeState::Working);
				TestEqual(TEXT("File state preserved"), State->State.FileState, Case.FileState);
				TestTrue(TEXT("Pending change remains visible"), State->IsModified());
			}
		}
	});

	It("should preserve staged-only, worktree-only, untracked and ignored states", [this]()
	{
		struct FCase { const TCHAR* Status; ETreeState::Type TreeState; };
		const FCase Cases[] = {{TEXT("M "), ETreeState::Staged}, {TEXT("A "), ETreeState::Staged},
			{TEXT(" M"), ETreeState::Working}, {TEXT("??"), ETreeState::Untracked}, {TEXT("!!"), ETreeState::Ignored}};
		const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		const FString File = Root / TEXT("Content/StatusFixture.uasset");
		for (const FCase& Case : Cases)
		{
			TMap<FString, FString> Results;
			Results.Add(File, FString(Case.Status) + TEXT(" Content/StatusFixture.uasset"));
			TMap<FString, FGitSourceControlState> States;
			GitSourceControlUtils::ParseStatusResults(TEXT(""), Root, false, {File}, Results, States);
			const FGitSourceControlState* State = States.Find(File);
			TestNotNull(Case.Status, State);
			if (State)
			{
				TestEqual(Case.Status, State->State.TreeState, Case.TreeState);
			}
		}
	});
}
#endif
