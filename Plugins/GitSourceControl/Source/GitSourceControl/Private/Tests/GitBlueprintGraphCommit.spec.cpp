// Copyright (c) 2026 ASGC
#if WITH_DEV_AUTOMATION_TESTS

#include "GitSourceControlUtils.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

BEGIN_DEFINE_SPEC(FGitBlueprintGraphCommitSpec, "Atlantis.Git.BlueprintGraphCommit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	FString Git;
	FString Root;
	FString Project;
	TArray<FString> Results;
	TArray<FString> Errors;

	bool Run(const FString& Command, const TArray<FString>& Parameters, const TArray<FString>& Files = {})
	{
		Results.Reset();
		Errors.Reset();
		return GitSourceControlUtils::RunCommand(Command, Git, Root, Parameters, Files, Results, Errors);
	}

	void Write(const FString& Path, const FString& Contents)
	{
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
		TestTrue(TEXT("Write fixture"), FFileHelper::SaveStringToFile(Contents, *Path));
	}

	bool Commit(TArray<FString>& Files)
	{
		if (!GitSourceControlUtils::AppendBlueprintGraphFiles(Git, Root, Project, Files, Errors))
		{
			return false;
		}
		return GitSourceControlUtils::RunCommit(Git, Root, {TEXT("-m"), TEXT("fixture")}, Files, Results, Errors);
	}
END_DEFINE_SPEC(FGitBlueprintGraphCommitSpec)

void FGitBlueprintGraphCommitSpec::Define()
{
	BeforeEach([this]()
	{
		Git = GitSourceControlUtils::FindGitBinaryPath();
		Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("GitBlueprintGraphTests") /
			FGuid::NewGuid().ToString(EGuidFormats::Digits));
		Project = Root / TEXT("Game Project");
		IFileManager::Get().MakeDirectory(*Root, true);
		TestTrue(TEXT("Initialize isolated local repository"), Run(TEXT("init"), {}));
		TestTrue(TEXT("Set fixture author"), Run(TEXT("config"), {TEXT("user.name"), TEXT("Fixture")}));
		TestTrue(TEXT("Set fixture email"), Run(TEXT("config"), {TEXT("user.email"), TEXT("fixture@example.invalid")}));
		TestTrue(TEXT("Disable signing in fixture"), Run(TEXT("config"), {TEXT("commit.gpgsign"), TEXT("false")}));
		TestTrue(TEXT("Disable external hooks in fixture"), Run(TEXT("config"), {TEXT("core.hooksPath"), TEXT(".disabled-hooks")}));
	});

	AfterEach([this]()
	{
		TestTrue(TEXT("Remove isolated fixture"), IFileManager::Get().DeleteDirectory(*Root, false, true));
	});

	It("should commit a selected asset and its new or modified export without unrelated staged exports", [this]()
	{
		const FString Asset = Project / TEXT("Content/UI/WBP_HUD_Root.uasset");
		const FString Graph = Project / TEXT("BlueprintGraphs/UI/WBP_HUD_Root.bpgraph.json");
		const FString Other = Project / TEXT("BlueprintGraphs/UI/Other.bpgraph.json");
		Write(Asset, TEXT("asset"));
		Write(Graph, TEXT("graph"));
		Write(Other, TEXT("unrelated"));
		TestTrue(TEXT("Stage unrelated graph"), Run(TEXT("add"), {}, {Other}));
		TArray<FString> Files {Asset};
		TestTrue(TEXT("Commit asset and export"), Commit(Files));
		TestEqual(TEXT("Only the pair is selected"), Files.Num(), 2);
		TestTrue(TEXT("Inspect committed paths"), Run(TEXT("show"), {TEXT("--pretty="), TEXT("--name-only"), TEXT("HEAD")}));
		const FString Committed = FString::Join(Results, TEXT("\n"));
		TestTrue(TEXT("Asset committed"), Committed.Contains(TEXT("WBP_HUD_Root.uasset")));
		TestTrue(TEXT("Graph committed"), Committed.Contains(TEXT("WBP_HUD_Root.bpgraph.json")));
		TestFalse(TEXT("Unrelated graph excluded"), Committed.Contains(TEXT("Other.bpgraph.json")));
		Write(Asset, TEXT("edited asset"));
		Write(Graph, TEXT("edited graph"));
		Files = {Asset};
		TestTrue(TEXT("Commit modified pair"), Commit(Files));
		TestTrue(TEXT("Inspect remaining changes"), Run(TEXT("status"), {TEXT("--porcelain")}));
		TestEqual(TEXT("Only unrelated change remains"), Results.Num(), 1);
		TestTrue(TEXT("Unrelated graph remains staged"), FString::Join(Results, TEXT("\n")).Contains(TEXT("Other.bpgraph.json")));
	});

	It("should include tracked export deletions whether staged or unstaged", [this]()
	{
		for (const bool bStageDeletion : {false, true})
		{
			const FString Name = bStageDeletion ? TEXT("Staged") : TEXT("Unstaged");
			const FString Asset = Project / TEXT("Content/Maps") / (Name + TEXT(".umap"));
			const FString Graph = Project / TEXT("BlueprintGraphs/Maps") / (Name + TEXT(".bpgraph.json"));
			Write(Asset, TEXT("map"));
			Write(Graph, TEXT("graph"));
			TArray<FString> Files {Asset};
			TestTrue(TEXT("Commit initial pair"), Commit(Files));
			TestTrue(TEXT("Delete map"), IFileManager::Get().Delete(*Asset));
			TestTrue(TEXT("Delete export"), IFileManager::Get().Delete(*Graph));
			if (bStageDeletion)
			{
				TestTrue(TEXT("Stage deletions"), Run(TEXT("add"), {TEXT("-A")}, {Asset, Graph}));
			}
			Files = {Asset};
			TestTrue(TEXT("Commit deletion pair"), Commit(Files));
			TestEqual(TEXT("Deleted export selected"), Files.Num(), 2);
			TestTrue(TEXT("Inspect remaining changes"), Run(TEXT("status"), {TEXT("--porcelain")}));
			TestTrue(TEXT("No export deletion left behind"), Results.IsEmpty());
		}
	});

	It("should ignore absent exports and files outside project Content and avoid duplicates", [this]()
	{
		const FString Asset = Project / TEXT("Content/UI/Present.uasset");
		const FString Graph = Project / TEXT("BlueprintGraphs/UI/Present.bpgraph.json");
		Write(Graph, TEXT("graph"));
		TArray<FString> Files {Asset, Graph, Project / TEXT("Content/UI/NoExport.uasset"), Root / TEXT("Elsewhere/Other.uasset")};
		TestTrue(TEXT("Resolve exports"), GitSourceControlUtils::AppendBlueprintGraphFiles(Git, Root, Project, Files, Errors));
		TestEqual(TEXT("No extra paths"), Files.Num(), 4);
		TArray<FString> NoFiles;
		TestTrue(TEXT("Push-only selection stays empty"), GitSourceControlUtils::AppendBlueprintGraphFiles(Git, Root, Project, NoFiles, Errors));
		TestTrue(TEXT("No implicit commit"), NoFiles.IsEmpty());
	});
}
#endif
