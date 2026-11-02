// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS

#include "GitSourceControlUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"

BEGIN_DEFINE_SPEC(FGitSystemLfsSpec, "Atlantis.Git.SystemLfs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FString Git;
	FString Root;
END_DEFINE_SPEC(FGitSystemLfsSpec)

void FGitSystemLfsSpec::Define()
{
	BeforeEach([this]()
	{
		Git = GitSourceControlUtils::FindGitBinaryPath();
		Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
	});

	It("should run the installed LFS through the configured Git executable", [this]()
	{
		TArray<FString> Results, Errors;
		TestTrue(TEXT("LFS version command"), GitSourceControlUtils::RunLFSCommand(TEXT("version"), Root, Git, {}, {}, Results, Errors));
		TestTrue(TEXT("LFS version output"), FString::Join(Results, TEXT("\n")).Contains(TEXT("git-lfs/")));
	});

	It("should propagate a failed LFS command", [this]()
	{
		TArray<FString> Results, Errors;
		TestFalse(TEXT("Invalid command fails"), GitSourceControlUtils::RunLFSCommand(TEXT("atlantis-invalid-command"), Root, Git, {}, {}, Results, Errors));
		TestFalse(TEXT("Error output is preserved"), Errors.IsEmpty());
	});

	// Explicit opt-in: creates and releases only unique disposable server locks.
	if (FParse::Param(FCommandLine::Get(), TEXT("GitLfsServerTests")))
	{
		It("should lock every selected path, report partial failure, and unlock every path", [this]()
		{
			const FString Directory = TEXT("Saved/GitLfsIntegration/") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
			const TArray<FString> Files = {Directory / TEXT("First asset.uasset"), Directory / TEXT("Second asset.uasset")};
			IFileManager::Get().MakeDirectory(*(Root / Directory), true);
			for (const FString& File : Files)
			{
				TestTrue(TEXT("Create disposable file"), FFileHelper::SaveStringToFile(TEXT("LFS integration fixture"), *(Root / File)));
			}
			TArray<FString> Results, Errors;
			auto Run = [&](const FString& Command, const TArray<FString>& Paths)
			{
				Results.Reset(); Errors.Reset();
				return GitSourceControlUtils::RunLFSCommand(Command, Root, Git, {}, Paths, Results, Errors);
			};
			auto ExpectLocks = [&](bool bPresent)
			{
				TestTrue(TEXT("Read remote locks"), Run(TEXT("locks"), {}));
				const FString Output = FString::Join(Results, TEXT("\n"));
				for (const FString& File : Files)
				{
					TestEqual(*File, Output.Contains(File), bPresent);
				}
			};
			TestTrue(TEXT("Batch lock"), Run(TEXT("lock"), Files));
			ExpectLocks(true);
			TestTrue(TEXT("Batch unlock"), Run(TEXT("unlock"), Files));
			ExpectLocks(false);
			TestTrue(TEXT("Cached lock query"), GitSourceControlUtils::RunLFSCommand(TEXT("locks"), Root, Git, {TEXT("--cached")}, {}, Results, Errors));
			TestTrue(TEXT("Local lock query"), GitSourceControlUtils::RunLFSCommand(TEXT("locks"), Root, Git, {TEXT("--local")}, {}, Results, Errors));
			TestTrue(TEXT("Single lock"), Run(TEXT("lock"), {Files[0]}));
			// Duplicate first lock fails; the second path must still be locked.
			TestFalse(TEXT("Partial failure is reported"), Run(TEXT("lock"), Files));
			ExpectLocks(true);
			TestTrue(TEXT("Cleanup locks"), Run(TEXT("unlock"), Files));
			ExpectLocks(false);
			for (const FString& File : Files)
			{
				TestTrue(TEXT("Remove disposable file"), IFileManager::Get().Delete(*(Root / File), false, true));
			}
			TestTrue(TEXT("Remove disposable directory"), IFileManager::Get().DeleteDirectory(*(Root / Directory)));
		});
	}
}
#endif
