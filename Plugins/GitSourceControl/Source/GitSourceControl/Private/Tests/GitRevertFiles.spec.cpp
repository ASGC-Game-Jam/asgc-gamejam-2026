// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS
#include "GitSourceControlUtils.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

BEGIN_DEFINE_SPEC(FGitRevertFilesSpec, "Atlantis.Git.RevertFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FString Git;
	FString Root;
	TArray<FString> Results;
	TArray<FString> Errors;
	bool Run(const FString& Command, const TArray<FString>& Parameters, const TArray<FString>& Files = {})
	{
		Results.Reset();
		Errors.Reset();
		return GitSourceControlUtils::RunCommand(Command, Git, Root, Parameters, Files, Results, Errors);
	}
	void Write(const FString& File, const FString& Text)
	{
		TestTrue(TEXT("Write fixture"), FFileHelper::SaveStringToFile(Text, *(Root / File)));
	}
	bool Revert(const TArray<FString>& Files)
	{
		return GitSourceControlUtils::RevertFiles(Git, Root, Files, Results, Errors);
	}
END_DEFINE_SPEC(FGitRevertFilesSpec)

void FGitRevertFilesSpec::Define()
{
	BeforeEach([this]()
	{
		Git = GitSourceControlUtils::FindGitBinaryPath();
		Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("GitRevertTests") /
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
		IFileManager::Get().MakeDirectory(*Root, true);
		TestTrue(TEXT("Initialize local repository"), Run(TEXT("init"), {}));
		TestTrue(TEXT("Set author"), Run(TEXT("config"), {TEXT("user.name"), TEXT("Fixture")}));
		TestTrue(TEXT("Set email"), Run(TEXT("config"), {TEXT("user.email"), TEXT("fixture@example.invalid")}));
		TestTrue(TEXT("Disable signing"), Run(TEXT("config"), {TEXT("commit.gpgsign"), TEXT("false")}));
		TestTrue(TEXT("Disable external hooks"), Run(TEXT("config"), {TEXT("core.hooksPath"), TEXT(".disabled-hooks")}));
		Write(TEXT("Selected asset.uasset"), TEXT("original"));
		Write(TEXT("Other.uasset"), TEXT("original"));
		TestTrue(TEXT("Add fixtures"), Run(TEXT("add"), {TEXT("-A")}));
		TestTrue(TEXT("Commit fixtures"), Run(TEXT("commit"), {TEXT("-m"), TEXT("fixture")}));
	});
	AfterEach([this]()
	{
		TestTrue(TEXT("Remove disposable repository"), IFileManager::Get().DeleteDirectory(*Root, false, true));
	});

	It("should restore staged and unstaged deletions without changing unrelated edits", [this]()
	{
		Write(TEXT("Other.uasset"), TEXT("unrelated"));
		const FString Asset = Root / TEXT("Selected asset.uasset");
		for (const bool bStaged : {false, true})
		{
			TestTrue(TEXT("Delete selected asset"), IFileManager::Get().Delete(*Asset));
			if (bStaged)
			{
				TestTrue(TEXT("Stage deletion"), Run(TEXT("add"), {TEXT("-A")}, {Asset}));
			}
			TestTrue(TEXT("Revert deleted asset"), Revert({Asset}));
			FString Contents;
			TestTrue(TEXT("Restored file exists"), FFileHelper::LoadFileToString(Contents, *Asset));
			TestEqual(TEXT("Restored HEAD content"), Contents, FString(TEXT("original")));
			TestTrue(TEXT("Inspect index"), Run(TEXT("diff"), {TEXT("--cached"), TEXT("--name-only")}));
			TestTrue(TEXT("Deletion no longer staged"), Results.IsEmpty());
		}
		FString Other;
		FFileHelper::LoadFileToString(Other, *(Root / TEXT("Other.uasset")));
		TestEqual(TEXT("Unrelated edit survives"), Other, FString(TEXT("unrelated")));
	});

	It("should restore both trees and unstage new files without deleting them", [this]()
	{
		Write(TEXT("Selected asset.uasset"), TEXT("staged edit"));
		TestTrue(TEXT("Stage edit"), Run(TEXT("add"), {}, {Root / TEXT("Selected asset.uasset")}));
		Write(TEXT("Selected asset.uasset"), TEXT("worktree edit"));
		Write(TEXT("New.uasset"), TEXT("new work"));
		TestTrue(TEXT("Stage new asset"), Run(TEXT("add"), {}, {Root / TEXT("New.uasset")}));
		TestTrue(TEXT("Revert selection"), Revert({Root / TEXT("Selected asset.uasset"), Root / TEXT("New.uasset")}));
		TestTrue(TEXT("Inspect tracked differences"), Run(TEXT("diff"), {TEXT("HEAD"), TEXT("--name-only")}));
		TestTrue(TEXT("Both trees match HEAD"), Results.IsEmpty());
		FString Contents;
		FFileHelper::LoadFileToString(Contents, *(Root / TEXT("New.uasset")));
		TestEqual(TEXT("New asset preserved locally"), Contents, FString(TEXT("new work")));
		TestTrue(TEXT("Inspect new file"), Run(TEXT("status"), {TEXT("--porcelain")}, {Root / TEXT("New.uasset")}));
		TestTrue(TEXT("New file is untracked"), FString::Join(Results, TEXT("\n")).StartsWith(TEXT("??")));
	});
}
#endif
