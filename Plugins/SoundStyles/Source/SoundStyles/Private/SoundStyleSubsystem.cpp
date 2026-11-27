// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT


#include "SoundStyleSubsystem.h"

#include "SoundStyleSettings.h"
#include "SoundStyleTable.h"
#include "SoundStyleTags.h"
#include "Framework/Application/SlateApplication.h"
#include "SoundStyleScopeInterface.h"
#include "Components/Widget.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "SoundStyleWidgetInterface.h"
#include "AudioModulationStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "Engine/GameInstance.h"   // GetGameInstance() as a world context (needs the full UGameInstance type)


DEFINE_LOG_CATEGORY(LogSoundStyles);

// Console: SoundStyles.Debug 1 logs every UI sound (widget, event, answering row, parameters sent).
static bool SoundStyleFindRowToEditCVar = false;
FAutoConsoleVariableRef CVArSoundStyleFindRowToEdit(
	TEXT("SoundStyles.Debug"),                  // the command name
	SoundStyleFindRowToEditCVar,                // the variable it controls
	TEXT("1 = log every UI sound: its classes, event and which style table row answered. Use it to find the row to edit.\n")
	TEXT("Default: 0"),                         // help text (shown in the console)
	ECVF_Default);

// After an Open: focus set by code in this window doesn't play Hover.
static constexpr double OpenSettleSeconds = 0.1;
// After a level load: all UI sounds are dropped in this window, while the new UI builds.
static constexpr double LoadSettleSeconds = 0.5;
// Loops fade out instead of stopping dead, so they don't click.
static constexpr float LoopFadeOutSeconds = 0.05f;

void USoundStyleSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const USoundStyleSettings* Settings = GetDefault<USoundStyleSettings>();

	StyleTable = Settings->StyleTable.LoadSynchronous();

	if (!StyleTable)
	{
		UE_LOG(LogSoundStyles, Error, TEXT("Sound Styles Table not set in Project Settings! (Project Settings > Game > Sound Styles > Style Table)"));
	}

	// Engine-wide level-load events. Keep the handles so Deinitialize can unbind.
	PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USoundStyleSubsystem::HandlePreLoadMap);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USoundStyleSubsystem::HandlePostLoadMap);
}

void USoundStyleSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);

	// Reverse path: the game is shutting down (or PIE stopped) with a screen still open or a widget still held.
	// Release the mixes and loops now, or the world could stay ducked on an audio device that outlives this game instance.
	ClearAllUIAudioStates();
	StopAllUILoops();

	Super::Deinitialize();
}

bool USoundStyleSubsystem::IsDebugEnabled()
{
	return SoundStyleFindRowToEditCVar;
}

const FSoundStyleEvent* USoundStyleSubsystem::Resolve(const TArray<FGameplayTagContainer>& Levels, FGameplayTag EventTag, FGameplayTag& OutRowTag) const
{
	// The lookup lives on the table (so the editor can use it too). No table: nothing answers.
	if (!StyleTable)
	{
		return nullptr;
	}
	return StyleTable->Resolve(Levels, EventTag, OutRowTag);
}

void USoundStyleSubsystem::PlayUISound(const FGameplayTagContainer& Classes, FGameplayTag EventTag)
{
	TArray<FGameplayTagContainer> Levels = {Classes};
	PlayLevels(Levels, EventTag, Classes.ToStringSimple());
}

void USoundStyleSubsystem::SetStyleTable(USoundStyleTable* InTable)
{
	StyleTable = InTable;
}

// --- Scopes ---
// A scope is any widget implementing ISoundStyleScopeInterface (e.g. WBP_PauseMenu returning {Menu}).
// Buttons inside it with no classes of their own use the scope's classes.

// Is this widget a scope? If so, OutClasses = its classes.
bool USoundStyleSubsystem::GetScopeClasses(const UObject* Widget, FGameplayTagContainer& OutClasses)
{
	if (!Widget || !Widget->Implements<USoundStyleScopeInterface>())
	{
		return false;
	}

	OutClasses = ISoundStyleScopeInterface::Execute_GetUISoundClasses(Widget);
	return true;
}

// The widget directly above this one, or nullptr at the very top.
const UWidget* USoundStyleSubsystem::GetParentWidget(const UWidget* Widget)
{
	if (!Widget)
	{
		return nullptr;
	}

	// Inside the same Widget Blueprint
	const UWidget* MaybeParent = Widget->GetParent();
	if (MaybeParent)
	{
		return MaybeParent;
	}

	// Top of this Widget Blueprint's tree: cross into the Widget Blueprint that owns it.
	// (widget -> Outer: WidgetTree -> Outer: UserWidget)
	const UWidgetTree* MaybeTree = Cast<UWidgetTree>(Widget->GetOuter());

	if (MaybeTree)
	{
		MaybeParent = Cast<UWidget>(MaybeTree->GetOuter());
		return MaybeParent;
	}
	return nullptr;
}

// The Levels list for Resolve: [0] = the widget's own classes, then each scope above it, nearest first.
// e.g. QuitButton in the pause menu -> [ {}, {Menu} ]
TArray<FGameplayTagContainer> USoundStyleSubsystem::BuildLevels(const UWidget* Widget, const FGameplayTagContainer& OwnClasses)
{
	TArray<FGameplayTagContainer> Levels;
	Levels.Add(OwnClasses);

	// Climb from the widget's parent to the top, one widget at a time; collect every scope passed.
	for (const UWidget* Current = GetParentWidget(Widget); Current; Current = GetParentWidget(Current))
	{
		FGameplayTagContainer ScopeClasses;
		if(GetScopeClasses(Current, ScopeClasses))
		{
			Levels.Add(ScopeClasses);
		}
	}

	return Levels;
}

bool USoundStyleSubsystem::CanPlayEvent(FGameplayTag EventTag)
{
	// Level loading or just loaded: drop everything (restored UI shouldn't chime).
	if (bLoadingMap || IsInLoadSettleWindow())
	{
		return false;
	}

	// Start the Open settle window (see IsInOpenSettleWindow), whichever play path the Open takes.
	if (EventTag == SoundStyles_Event_Open)
	{
		LastOpenTime = FPlatformTime::Seconds();
	}
	return true;
}

void USoundStyleSubsystem::PlayLevels(const TArray<FGameplayTagContainer>& Levels, FGameplayTag EventTag, const FString& DebugName)
{
	if (!CanPlayEvent(EventTag))
	{
		return;
	}

	FGameplayTag FoundTag;
	const FSoundStyleEvent* FoundEvent = Resolve(Levels, EventTag, FoundTag);
	if (SoundStyleFindRowToEditCVar)
	{
		UE_LOG(LogSoundStyles, Display, TEXT("%s / %s -> row %s"),
				*DebugName,   // 1st %s: DebugName
				*EventTag.ToString(),        // 2nd %s: event
				*FoundTag.ToString());       // 3rd %s: row that answered
	}

	// Nothing answered, the answer is silent, or no Slate (tests, commandlets).
	if(!FoundEvent || !FoundEvent->Sound || !FSlateApplication::IsInitialized())
	{
		return;
	}

	// The table gives this event parameters: Slate can't carry them, so play through an audio component instead.
	// The game instance is the world context (PlayUISound has no widget).
	// Persist across level transition, like the Slate path: a press that starts a level load must not be cut off.
	if (!FoundEvent->Parameters.IsEmpty())
	{
		UAudioComponent* OneShot = UGameplayStatics::CreateSound2D(GetGameInstance(), FoundEvent->Sound,
			1.f, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition*/ true);
		if (OneShot)
		{
			OneShot->SetParameters_Blueprint(FoundEvent->Parameters);
			LogParameters(TEXT("table"), FoundEvent->Parameters);
			OneShot->Play();
		}
		return;   // never play twice: the Slate block below is skipped
	}

	// Slate plays with no world: survives level loads, plays while paused. Sound Concurrency applies.
	FSlateSound SlateSound;
	SlateSound.SetResourceObject(FoundEvent->Sound);
	FSlateApplication::Get().PlaySound(SlateSound);
}

TArray<FGameplayTagContainer> USoundStyleSubsystem::GetWidgetLevels(const UWidget* Widget)
{
	// Own classes: a Sound Style widget's Sound Classes (Button, Slider...), otherwise its scope classes (if it is a scope).
	FGameplayTagContainer OwnClasses;
	GetScopeClasses(Widget, OwnClasses);

	const ISoundStyleWidgetInterface* SoundStyleWidget = Cast<ISoundStyleWidgetInterface>(Widget);
	if (SoundStyleWidget)
	{
		OwnClasses = SoundStyleWidget->GetOwnSoundClasses();
	}

	return BuildLevels(Widget, OwnClasses);
}

void USoundStyleSubsystem::PlayWidgetUISound(const UWidget* Widget, FGameplayTag EventTag)
{
	if (!Widget)
	{
		return;
	}

	PlayLevels(GetWidgetLevels(Widget), EventTag, Widget->GetName());
}


void USoundStyleSubsystem::LogParameters(const FString& Source, const TArray<FAudioParameter>& Parameters)
{
	if (!SoundStyleFindRowToEditCVar || Parameters.IsEmpty())
	{
		return;
	}

	// "Value=0.37, Brightness=0.20". Floats show their value; other types just their name.
	FString Result;
	for (const FAudioParameter& Param : Parameters)
	{
		if (!Result.IsEmpty())
		{
			Result += TEXT(", ");
		}
		Result += Param.ParamName.ToString();

		if (Param.ParamType == EAudioParameterType::Float)
		{
			Result += FString::Printf(TEXT("=%.2f"), Param.FloatParam);
		}
	}

	UE_LOG(LogSoundStyles, Display, TEXT("    parameters from %s: %s"), *Source, *Result);
}


UAudioComponent* USoundStyleSubsystem::CreateWidgetSound(const UWidget* Widget, FGameplayTag EventTag, const FString& DebugLabel, bool bPersistAcrossLevelTransition)
{
	if (!Widget || !CanPlayEvent(EventTag))
	{
		return nullptr;
	}

	FGameplayTag FoundTag;
	const FSoundStyleEvent* FoundEvent = Resolve(GetWidgetLevels(Widget), EventTag, FoundTag);
	if (SoundStyleFindRowToEditCVar)
	{
		UE_LOG(LogSoundStyles, Display, TEXT("%s / %s (%s) -> row %s"), *Widget->GetName(), *EventTag.ToString(), *DebugLabel, *FoundTag.ToString());
	}

	if (!FoundEvent || !FoundEvent->Sound)
	{
		return nullptr;
	}

	// Not playing yet. CreateSound2D also sets bIsUISound (plays while paused) and bAutoDestroy (deletes itself when done).
	UAudioComponent* Component = UGameplayStatics::CreateSound2D(Widget, FoundEvent->Sound,
		1.f, 1.f, 0.f, nullptr, bPersistAcrossLevelTransition);
	if (!Component)
	{
		return nullptr;
	}

	// The table's fixed parameters first. Callers send live game values after, and those win by name.
	Component->SetParameters_Blueprint(FoundEvent->Parameters);
	LogParameters(TEXT("table"), FoundEvent->Parameters);
	return Component;
}

void USoundStyleSubsystem::PlayWidgetUISoundWithParameters(const UWidget* Widget, FGameplayTag EventTag, const TArray<FAudioParameter>& Parameters)
{
	// One-shots persist across level loads, like the Slate path.
	UAudioComponent* OneShot = CreateWidgetSound(Widget, EventTag, TEXT("with parameters"), /*bPersistAcrossLevelTransition*/ true);
	if (!OneShot)
	{
		return;
	}

	// Parameters BEFORE Play: they're stored, then sent with the start, so the first sample already has them.
	OneShot->SetParameters_Blueprint(Parameters);
	LogParameters(TEXT("game"), Parameters);
	OneShot->Play();
}

// --- Loops ---
// One card per widget in ActiveLoops; the UPROPERTY keeps each component alive while it plays.

void USoundStyleSubsystem::StartUILoop(const UWidget* Widget, FGameplayTag EventTag)
{
	// Same as the "with parameters" version, with no game parameters (the table's still apply).
	StartUILoopWithParameters(Widget, EventTag, TArray<FAudioParameter>());
}

void USoundStyleSubsystem::StartUILoopWithParameters(const UWidget* Widget, FGameplayTag EventTag, const TArray<FAudioParameter>& Parameters)
{
	RemoveOrphanedLoops();
	StopUILoop(Widget);   // one loop per widget

	// Loops don't persist: they belong to a widget, and HandlePreLoadMap stops them all anyway.
	UAudioComponent* Loop = CreateWidgetSound(Widget, EventTag, TEXT("loop"), /*bPersistAcrossLevelTransition*/ false);
	if (!Loop)
	{
		return;
	}

	// Parameters BEFORE Play, so the loop starts at the right place.
	Loop->SetParameters_Blueprint(Parameters);
	LogParameters(TEXT("game"), Parameters);
	Loop->Play();

	FSoundStyleActiveLoop NewLoop;
	NewLoop.Widget = Widget;
	NewLoop.Component = Loop;
	ActiveLoops.Add(NewLoop);
}

void USoundStyleSubsystem::StopUILoop(const UWidget* Widget)
{
	// A null widget must not match a card whose widget has been destroyed (its weak pointer also reads as null).
	if (!Widget)
	{
		return;
	}

	// Index loop (not range-for) because we remove from the array we're walking.
	for (int32 Index = 0; Index < ActiveLoops.Num(); Index++)
	{
		if (ActiveLoops[Index].Widget == Widget)
		{
			if (ActiveLoops[Index].Component)
			{
				ActiveLoops[Index].Component->FadeOut(LoopFadeOutSeconds, 0.f);
			}
			ActiveLoops.RemoveAt(Index);
			return;
		}
	}
}

void USoundStyleSubsystem::RemoveOrphanedLoops()
{
	// Normally a widget stops its loop when it's torn down (ReleaseSlateResources). This is the safety net:
	// a card whose widget is gone would otherwise keep its loop playing until the next level load.
	ActiveLoops.RemoveAll([](const FSoundStyleActiveLoop& Loop)
	{
		const bool bWidgetGone = !Loop.Widget.IsValid();
		if (bWidgetGone && Loop.Component)
		{
			Loop.Component->FadeOut(LoopFadeOutSeconds, 0.f);
		}
		return bWidgetGone;
	});
}

void USoundStyleSubsystem::StopAllUILoops()
{
	for (const FSoundStyleActiveLoop& Loop : ActiveLoops)
	{
		// IsValid, not just non-null: during shutdown the component may already be being destroyed with its world.
		if (IsValid(Loop.Component))
		{
			Loop.Component->Stop();
		}
	}
	ActiveLoops.Empty();
}

UAudioComponent* USoundStyleSubsystem::FindUILoop(const UWidget* Widget) const
{
	for (const FSoundStyleActiveLoop& Loop : ActiveLoops)
	{
		if(Loop.Widget == Widget)
		{
			return Loop.Component;
		}
	}
	return nullptr;
}

void USoundStyleSubsystem::SetUILoopParameters(const UWidget* Widget, const TArray<FAudioParameter>& Parameters)
{
	UAudioComponent* Loop = FindUILoop(Widget);
	if (!Loop)
	{
		return;   // no loop playing for this widget (not held, or no sound for the event)
	}

	// Not logged: callers may send this every frame.
	Loop->SetParameters_Blueprint(Parameters);
}

bool USoundStyleSubsystem::IsInOpenSettleWindow() const
{
	double Now = FPlatformTime::Seconds();
	return Now - LastOpenTime < OpenSettleSeconds;
}

bool USoundStyleSubsystem::IsInLoadSettleWindow() const
{
	double Now = FPlatformTime::Seconds();
	return Now - LastLoadEndTime < LoadSettleSeconds;
}

// --- Screen audio states ---
// Each mix is reference-counted, so two screens sharing one mix don't cancel each other.

void USoundStyleSubsystem::PushMix(USoundControlBusMix* Mix)
{
	if (!Mix)
	{
		return;
	}

	int32& Count = ActiveMixCounts.FindOrAdd(Mix);
	if (Count == 0)
	{
		UAudioModulationStatics::ActivateBusMix(GetGameInstance(), Mix);
	}

	Count++;
}

void USoundStyleSubsystem::PopMix(USoundControlBusMix* Mix)
{
	if (!Mix)
	{
		return;
	}

	int32* Count = ActiveMixCounts.Find(Mix);
	if (!Count)
	{
		return;
	}

	(*Count)--;

	if(*Count == 0)
	{
		UAudioModulationStatics::DeactivateBusMix(GetGameInstance(), Mix);
		ActiveMixCounts.Remove(Mix);
	}
}

USoundControlBusMix* USoundStyleSubsystem::FindStateMix(FGameplayTag StateClass) const
{
	if (!StyleTable)
	{
		return nullptr;
	}

	const FSoundStyleClass* Row = StyleTable->Classes.Find(StateClass);

	if(!Row || !Row->WhileOpenMix)
	{
		UE_LOG(LogSoundStyles, Warning, TEXT("UI audio state '%s' has no While Open Mix in the style table."), *StateClass.ToString());
		return nullptr;
	}

	return Row->WhileOpenMix;
}

void USoundStyleSubsystem::PushUIAudioState(FGameplayTag StateClass)
{
	PushMix(FindStateMix(StateClass));
}

void USoundStyleSubsystem::PopUIAudioState(FGameplayTag StateClass)
{
	PopMix(FindStateMix(StateClass));
}

void USoundStyleSubsystem::ClearAllUIAudioStates()
{
	for(const auto& Pair : ActiveMixCounts)
	{
		UAudioModulationStatics::DeactivateBusMix(GetGameInstance(), Pair.Key);
	}
	ActiveMixCounts.Empty();
}

int32 USoundStyleSubsystem::GetMixCount(const USoundControlBusMix* Mix) const
{
	if (!Mix)
	{
		return 0;
	}

	const int32* Count = ActiveMixCounts.Find(Mix);
	return Count ? *Count : 0;
}

// --- Level loading ---

void USoundStyleSubsystem::HandlePreLoadMap(const FString& MapName)
{
	// The old level's screens are destroyed without calling Pop, so release their mixes here.
	ClearAllUIAudioStates();
	StopAllUILoops();

	bLoadingMap = true;
}

void USoundStyleSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	bLoadingMap = false;
	LastLoadEndTime = FPlatformTime::Seconds();
}
