// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "GitSourceControlProvider.h"
#include "GitSourceControlChangelistState.h"
#include "GitSourceControlState.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

BEGIN_DEFINE_SPEC(FGitChangelistSubmitSpec, "Atlantis.Git.ChangelistSubmit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FGitChangelistSubmitSpec)

void FGitChangelistSubmitSpec::Define()
{
	It("should resolve only the submitted changelist and preserve explicit selections", [this]()
	{
		FGitSourceControlProvider Provider;
		const FString WorkingFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content/Working.uasset"));
		const FString StagedFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Content/Staged.uasset"));
		const auto Working = Provider.GetStateInternal(FGitSourceControlChangelist::WorkingChangelist);
		Working->Files.Add(Provider.GetStateInternal(WorkingFile));
		Provider.GetStateInternal(FGitSourceControlChangelist::StagedChangelist)->Files.Add(Provider.GetStateInternal(StagedFile));
		TArray<FString> Files;
		FText Error;
		TestTrue(TEXT("Resolve Working"), Provider.ResolveCheckInFiles(Working->GetChangelist(), Files, Error));
		TestEqual(TEXT("Only Working selected"), Files.Num(), 1);
		TestEqual(TEXT("Working path"), Files[0], WorkingFile);
		Files = {StagedFile};
		TestTrue(TEXT("Preserve explicit files"), Provider.ResolveCheckInFiles(Working->GetChangelist(), Files, Error));
		TestEqual(TEXT("Explicit path wins"), Files[0], StagedFile);
	});

	It("should reject an empty changelist but allow an explicit push-only request", [this]()
	{
		FGitSourceControlProvider Provider;
		TArray<FString> Files;
		FText Error;
		const auto Working = Provider.GetStateInternal(FGitSourceControlChangelist::WorkingChangelist);
		TestFalse(TEXT("Empty submission fails"), Provider.ResolveCheckInFiles(Working->GetChangelist(), Files, Error));
		TestFalse(TEXT("Error explains failure"), Error.IsEmpty());
		TestTrue(TEXT("Push-only allowed"), Provider.ResolveCheckInFiles(nullptr, Files, Error));
		TestTrue(TEXT("Push-only has no files"), Files.IsEmpty());
	});
}
#endif
