// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "Misc/AutomationTest.h"
#include "SoundStyleTags.h"
#include "SoundStyleSubsystem.h"
#include "SoundStyleTable.h"
#include "NativeGameplayTags.h"
#include "Engine/GameInstance.h"
#include "Sound/SoundWave.h"
#include "SoundControlBusMix.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesTagsRootsExistTest, "SoundStyles.Tags.RootsExist", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveExactMatchTest, "SoundStyles.Resolve.ExactMatch", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveParentFallbackTest, "SoundStyles.Resolve.ParentFallback", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveRootDefaultTest, "SoundStyles.Resolve.RootDefault", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveSilentStopsTest, "SoundStyles.Resolve.SilentStops", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveDeepestWinsTest, "SoundStyles.Resolve.DeepestWins", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveRowWithoutEventTest, "SoundStyles.Resolve.RowWithoutEvent", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveInvalidClassIgnoredTest, "SoundStyles.Resolve.InvalidClassIgnored", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveScopeFallbackTest, "SoundStyles.Resolve.ScopeFallback", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesResolveOwnBeatsScopeTest, "SoundStyles.Resolve.OwnBeatsScope", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesStatesStackingTest, "SoundStyles.States.Stacking", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesStatesSharedMixTest, "SoundStyles.States.SharedMix", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesStatesPopWithoutPushTest, "SoundStyles.States.PopWithoutPush", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSoundStylesStatesClearAllTest, "SoundStyles.States.ClearAll", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

UE_DEFINE_GAMEPLAY_TAG_STATIC(UISoundClassTest, "UISound.Class.Test");
UE_DEFINE_GAMEPLAY_TAG_STATIC(UISoundClassTestChild, "UISound.Class.Test.Child");

// A tag OUTSIDE UISound.Class, used only by InvalidClassIgnored.
// Deliberately 4 levels deep (deeper than Test's 3) so it would WIN if the guard didn't skip it.
// Kept outside UISound.* so it doesn't show up in the sound class/event tag pickers.
UE_DEFINE_GAMEPLAY_TAG_STATIC(NotASoundClassTag, "SoundStylesTest.Not.A.Class");

bool FSoundStylesTagsRootsExistTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("UISound.Class root tag is registered"), FGameplayTag(SoundStyles_Class).IsValid());
	TestTrue(TEXT("UISound.Event root tag is registered "), FGameplayTag(SoundStyles_Event).IsValid());
	return true;
}

bool FSoundStylesResolveExactMatchTest::RunTest(const FString& Parameters)
{
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass Row;
	Row.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());
	Asset->Classes.Add(UISoundClassTest, Row);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);


	TArray<FGameplayTagContainer> Levels = {FGameplayTagContainer(UISoundClassTest)};
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);

	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Answered by the UISound.Class.Test row"), FoundTag == UISoundClassTest);
	return true;
}

bool FSoundStylesResolveParentFallbackTest::RunTest(const FString& Parameters)
{
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass Row;
	Row.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());
	Asset->Classes.Add(UISoundClassTest, Row);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);


	TArray<FGameplayTagContainer> Levels = {FGameplayTagContainer(UISoundClassTestChild)};
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);

	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Child inherits Press from the UISound.Class.Test row"), FoundTag == UISoundClassTest);
	return true;
}

bool FSoundStylesResolveRootDefaultTest::RunTest(const FString& Parameters)
{
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass Row;
	Row.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());
	Asset->Classes.Add(SoundStyles_Class, Row);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);


	TArray<FGameplayTagContainer> Levels = {FGameplayTagContainer(UISoundClassTestChild)};
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);

	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Falls back to the UISound.Class root row"), FoundTag == SoundStyles_Class);
	return true;
}

bool FSoundStylesResolveSilentStopsTest::RunTest(const FString& Parameters)
{
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();

	USoundWave* FakeSound = NewObject<USoundWave>();

	FSoundStyleEvent LoudPress;
	LoudPress.Sound = FakeSound;
	FSoundStyleClass RootRow;
	RootRow.Events.Add(SoundStyles_Event_Press, LoudPress);

	FSoundStyleClass TestRow;
	TestRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	Asset->Classes.Add(SoundStyles_Class, RootRow);
	Asset->Classes.Add(UISoundClassTest, TestRow);


	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);


	TArray<FGameplayTagContainer> Levels = {FGameplayTagContainer(UISoundClassTestChild)};
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);


	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Silent Test row answered (search stopped before the root)"), FoundTag == UISoundClassTest);
	if (FoundEvent)
	{
		TestNull(TEXT("Test row's Press is silent"), FoundEvent->Sound.Get());
	}

	return true;
}

bool FSoundStylesResolveDeepestWinsTest::RunTest(const FString& Parameters)
{
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();

	FSoundStyleClass TestRow;
	TestRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	FSoundStyleClass ChildRow;
	ChildRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	Asset->Classes.Add(UISoundClassTest, TestRow);
	Asset->Classes.Add(UISoundClassTestChild, ChildRow);


	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	FGameplayTagContainer TwoClasses;
	TwoClasses.AddTag(UISoundClassTest);
	TwoClasses.AddTag(UISoundClassTestChild);

	TArray<FGameplayTagContainer> Levels = {TwoClasses};
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);


	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Deeper Child row wins over Test row"), FoundTag == UISoundClassTestChild);

	return true;
}

bool FSoundStylesResolveRowWithoutEventTest::RunTest(const FString& Parameters)
{
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();

	FSoundStyleClass TestRow;
	TestRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	FSoundStyleClass ChildRow;
	ChildRow.Events.Add(SoundStyles_Event_Hover, FSoundStyleEvent());

	Asset->Classes.Add(UISoundClassTest, TestRow);
	Asset->Classes.Add(UISoundClassTestChild, ChildRow);


	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	TArray<FGameplayTagContainer> Levels = { FGameplayTagContainer(UISoundClassTestChild) };

	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);


	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Child row has no Press, so the search continues to the Test row"), FoundTag == UISoundClassTest);

	return true;
}

// GIVEN  Test row: Press.  A row filed under a NON-class tag (SoundStylesTest.Not.A.Class): Press.
// WHEN   one level with the non-class tag AND Test is pressed
// THEN   the Test row answers, and a warning names the bad tag
// TRAP   the bad tag's row is deeper (4 vs 3). Without the MatchesTag guard in FindInClasses,
//        it would be looked up, answer, and win "deepest wins".
bool FSoundStylesResolveInvalidClassIgnoredTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();

	FSoundStyleClass TestRow;
	TestRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	FSoundStyleClass BadRow;
	BadRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	Asset->Classes.Add(UISoundClassTest, TestRow);
	Asset->Classes.Add(NotASoundClassTag, BadRow);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	// The guard logs a warning for the bad tag. Tell the test framework to expect it exactly once,
	// so the warning counts as correct behaviour (and its absence counts as a failure).
	AddExpectedMessage(TEXT("is not under UISound.Class"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1, false);

	// WHEN
	FGameplayTagContainer BadAndGood;
	BadAndGood.AddTag(NotASoundClassTag);
	BadAndGood.AddTag(UISoundClassTest);

	TArray<FGameplayTagContainer> Levels = { BadAndGood };
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);

	// THEN
	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Non-class tag is skipped, so the Test row answers"), FoundTag == UISoundClassTest);

	return true;
}

// GIVEN  Test row: Press.
// WHEN   level [0] (the widget's own classes) is EMPTY, level [1] (the screen's scope) is Test
// THEN   the Test row answers, from the scope
// TRAP   if Resolve only checked level [0], it would find nothing (there's no root row to fall back to either).
//        This is the "button with no classes uses the screen's scope" case from the design doc.
bool FSoundStylesResolveScopeFallbackTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();

	FSoundStyleClass TestRow;
	TestRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	Asset->Classes.Add(UISoundClassTest, TestRow);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	// WHEN: two LEVELS this time, not two classes in one level
	FGameplayTagContainer OwnClasses;                                // [0] widget's own classes: none
	FGameplayTagContainer ScopeClasses(UISoundClassTest);            // [1] the screen's scope: Test

	TArray<FGameplayTagContainer> Levels = { OwnClasses, ScopeClasses };
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);

	// THEN
	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Widget has no classes, so the scope's Test row answers"), FoundTag == UISoundClassTest);

	return true;
}

// GIVEN  Test row: Press.  Child row: Press.
// WHEN   level [0] (widget's own classes) is Test, level [1] (scope) is Child
// THEN   the Test row answers - the widget's own class is checked first
// TRAP   Child is DEEPER (4 vs 3). If levels were merged into one big list, "deepest wins" would
//        pick Child. Order of levels must beat depth.
bool FSoundStylesResolveOwnBeatsScopeTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundStyleTable* Asset = NewObject<USoundStyleTable>();

	FSoundStyleClass TestRow;
	TestRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	FSoundStyleClass ChildRow;
	ChildRow.Events.Add(SoundStyles_Event_Press, FSoundStyleEvent());

	Asset->Classes.Add(UISoundClassTest, TestRow);
	Asset->Classes.Add(UISoundClassTestChild, ChildRow);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	// WHEN
	FGameplayTagContainer OwnClasses(UISoundClassTest);              // [0] widget's own class: Test
	FGameplayTagContainer ScopeClasses(UISoundClassTestChild);       // [1] the screen's scope: Child (deeper!)

	TArray<FGameplayTagContainer> Levels = { OwnClasses, ScopeClasses };
	FGameplayTag FoundTag;

	const FSoundStyleEvent* FoundEvent = Sub->Resolve(Levels, SoundStyles_Event_Press, FoundTag);

	// THEN
	TestNotNull(TEXT("Press was found"), FoundEvent);
	TestTrue(TEXT("Widget's own Test class beats the deeper Child scope"), FoundTag == UISoundClassTest);

	return true;
}

// ===== Screen audio states (Push / Pop / Clear) =====
// These use an EMPTY mix (no stages), so activating it changes no audio.
// Each test ends with ClearAllUIAudioStates() so nothing stays active after the test.

// GIVEN  a row Test whose While Open Mix is Mix
// WHEN   Push Test twice, then Pop once
// THEN   the mix is still in use (count 1)
// TRAP   a version that deactivates on the first Pop would give 0
bool FSoundStylesStatesStackingTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundControlBusMix* Mix = NewObject<USoundControlBusMix>();

	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass Row;
	Row.WhileOpenMix = Mix;
	Asset->Classes.Add(UISoundClassTest, Row);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	TestEqual(TEXT("Starts inactive"), Sub->GetMixCount(Mix), 0);

	// WHEN / THEN, step by step
	Sub->PushUIAudioState(UISoundClassTest);
	TestEqual(TEXT("First push activates"), Sub->GetMixCount(Mix), 1);

	Sub->PushUIAudioState(UISoundClassTest);
	TestEqual(TEXT("Second push stacks"), Sub->GetMixCount(Mix), 2);

	Sub->PopUIAudioState(UISoundClassTest);
	TestEqual(TEXT("One pop leaves it active"), Sub->GetMixCount(Mix), 1);

	Sub->PopUIAudioState(UISoundClassTest);
	TestEqual(TEXT("Last pop deactivates"), Sub->GetMixCount(Mix), 0);

	Sub->ClearAllUIAudioStates();
	return true;
}

// GIVEN  two rows, Test and Test.Child, that BOTH use the same Mix
// WHEN   Push Test, Push Child, then Pop Test
// THEN   the mix is still in use (count 1) - Child still needs it
// TRAP   counting per STATE instead of per MIX would deactivate it here
bool FSoundStylesStatesSharedMixTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundControlBusMix* Mix = NewObject<USoundControlBusMix>();

	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass TestRow;
	TestRow.WhileOpenMix = Mix;
	FSoundStyleClass ChildRow;
	ChildRow.WhileOpenMix = Mix;   // same mix as TestRow
	Asset->Classes.Add(UISoundClassTest, TestRow);
	Asset->Classes.Add(UISoundClassTestChild, ChildRow);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	// WHEN / THEN
	Sub->PushUIAudioState(UISoundClassTest);
	Sub->PushUIAudioState(UISoundClassTestChild);
	TestEqual(TEXT("Two states share one mix"), Sub->GetMixCount(Mix), 2);

	Sub->PopUIAudioState(UISoundClassTest);
	TestEqual(TEXT("Child still holds the shared mix"), Sub->GetMixCount(Mix), 1);

	Sub->PopUIAudioState(UISoundClassTestChild);
	TestEqual(TEXT("Both popped: mix released"), Sub->GetMixCount(Mix), 0);

	Sub->ClearAllUIAudioStates();
	return true;
}

// GIVEN  a row Test with a Mix, never pushed
// WHEN   Pop Test (twice, to be sure)
// THEN   count stays 0
// TRAP   a Pop with no guard would make the count -1, and the NEXT push would then not activate
bool FSoundStylesStatesPopWithoutPushTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundControlBusMix* Mix = NewObject<USoundControlBusMix>();

	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass Row;
	Row.WhileOpenMix = Mix;
	Asset->Classes.Add(UISoundClassTest, Row);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	// WHEN
	Sub->PopUIAudioState(UISoundClassTest);
	Sub->PopUIAudioState(UISoundClassTest);

	// THEN
	TestEqual(TEXT("Pop without push does nothing"), Sub->GetMixCount(Mix), 0);

	// ...and a push afterwards still works normally
	Sub->PushUIAudioState(UISoundClassTest);
	TestEqual(TEXT("Push after stray pops activates"), Sub->GetMixCount(Mix), 1);

	Sub->ClearAllUIAudioStates();
	return true;
}

// GIVEN  two rows with DIFFERENT mixes, both pushed (one of them twice)
// WHEN   ClearAllUIAudioStates
// THEN   both counts are 0, and a stray Pop afterwards keeps them at 0
// TRAP   deactivating the mixes but forgetting to Empty() the map would leave the old counts behind
bool FSoundStylesStatesClearAllTest::RunTest(const FString& Parameters)
{
	// GIVEN
	USoundControlBusMix* MixA = NewObject<USoundControlBusMix>();
	USoundControlBusMix* MixB = NewObject<USoundControlBusMix>();

	USoundStyleTable* Asset = NewObject<USoundStyleTable>();
	FSoundStyleClass TestRow;
	TestRow.WhileOpenMix = MixA;
	FSoundStyleClass ChildRow;
	ChildRow.WhileOpenMix = MixB;
	Asset->Classes.Add(UISoundClassTest, TestRow);
	Asset->Classes.Add(UISoundClassTestChild, ChildRow);

	UGameInstance* GI = NewObject<UGameInstance>();
	USoundStyleSubsystem* Sub = NewObject<USoundStyleSubsystem>(GI);
	Sub->SetStyleTable(Asset);

	Sub->PushUIAudioState(UISoundClassTest);
	Sub->PushUIAudioState(UISoundClassTest);
	Sub->PushUIAudioState(UISoundClassTestChild);

	// WHEN
	Sub->ClearAllUIAudioStates();

	// THEN
	TestEqual(TEXT("Mix A cleared"), Sub->GetMixCount(MixA), 0);
	TestEqual(TEXT("Mix B cleared"), Sub->GetMixCount(MixB), 0);

	// The menu's own Pop may still arrive after a clear - it must not break anything
	Sub->PopUIAudioState(UISoundClassTest);
	TestEqual(TEXT("Pop after clear stays at 0"), Sub->GetMixCount(MixA), 0);

	return true;
}

#endif


