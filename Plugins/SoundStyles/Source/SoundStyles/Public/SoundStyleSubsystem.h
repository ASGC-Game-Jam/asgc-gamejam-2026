// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "AudioParameter.h"

#include "SoundStyleSubsystem.generated.h"

class USoundStyleTable;
struct FSoundStyleEvent;
class UWidget;
class USoundControlBusMix;
class UAudioComponent;

/** Log category for everything Sound Styles prints. Filter the Output Log by "LogSoundStyles". */
SOUNDSTYLES_API DECLARE_LOG_CATEGORY_EXTERN(LogSoundStyles, Log, All);

/** One playing UI loop: which widget it belongs to, and the audio component playing it. */
USTRUCT()
struct FSoundStyleActiveLoop
{
	GENERATED_BODY()

	/** The widget being held. Weak: the loop must not keep a closed screen alive. */
	TWeakObjectPtr<const UWidget> Widget;

	/** The playing loop. UPROPERTY (strong) so it isn't garbage collected while it plays. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> Component;
};

/**
 * Plays UI sounds: resolves sound classes against the style table.
 * Also runs UI loops (Hold, Fill), sends game data to MetaSound parameters, and keeps the screen audio state stack.
 *
 * Lives as long as the game instance (main menu through every level).
 * Created automatically by Unreal - get it with "Get SoundStyleSubsystem" in Blueprint.
 *
 * Lookup order for one event (first answer wins):
 *   1. Each level in order (widget's own classes, then scopes), walking up parent tags
 *   2. The UISound.Class root row (game-wide default)
 *   3. Nothing found - play nothing
 */
UCLASS()
class SOUNDSTYLES_API USoundStyleSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Runs once when the game instance starts. Loads the style table set in Project Settings > Game > Sound Styles. Logs an error if none is set. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Runs once when the game instance shuts down. */
	virtual void Deinitialize() override;

	/**
	 * Finds which row answers for an event. The full lookup: levels in order, then the root row.
	 * A silent event (row lists the event, Sound is empty) still counts as an answer and stops the search.
	 *
	 * @param Levels     Classes to check, in priority order. [0] = widget's own classes, [1] = nearest scope, ...
	 * @param EventTag   What happened, e.g. UISound.Event.Press.
	 * @param OutRowTag  Set to the class tag of the row that answered (UISound.Class if the root answered). Left empty if nothing answered.
	 * @return The event settings from the answering row, or nullptr if no row sets this event.
	 */
	const FSoundStyleEvent* Resolve(const TArray<FGameplayTagContainer>& Levels, FGameplayTag EventTag, FGameplayTag& OutRowTag) const;

	/**
	 * Plays the UI sound for these classes and event. Usually through Slate (no world: keeps playing while paused and across
	 * level loads); through an audio component instead if the table gives the event Parameters (Slate can't carry them).
	 * Plays nothing if no row sets the event, the event is silent, a level is loading, or Slate isn't running.
	 * With SoundStyles.Debug 1, logs the classes, event and which row answered.
	 *
	 * @param Classes   The widget's sound classes, e.g. UISound.Class.Menu.Back.
	 * @param EventTag  What happened, e.g. UISound.Event.Press.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles")
	void PlayUISound(const FGameplayTagContainer& Classes, FGameplayTag EventTag);

	/**
	 * Plays the UI sound for a widget: uses its own classes (a Sound Style widget's Sound Style property, or its scope interface),
	 * then each scope above it. Same rules as Play UI Sound.
	 *
	 * @param Widget    The widget the event happened on. Defaults to Self in Blueprint.
	 * @param EventTag  What happened, e.g. UISound.Event.Hover.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles", meta = (DefaultToSelf = "Widget"))
	void PlayWidgetUISound(const UWidget* Widget, FGameplayTag EventTag);


	/**
	 * Like Play Widget UI Sound, but sends game data to the sound (e.g. a score tick whose pitch rises with the score).
	 * Plays through an audio component instead of Slate, because only audio components can take parameters.
	 *
	 * @param Widget      The widget the event happened on. Defaults to Self in Blueprint.
	 * @param EventTag    What happened, e.g. UISound.Event.Press.
	 * @param Parameters  e.g. [Value = 0.7]. Names must match the MetaSound's inputs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles", meta = (DefaultToSelf = "Widget"))
	void PlayWidgetUISoundWithParameters(const UWidget* Widget, FGameplayTag EventTag, const TArray<FAudioParameter>& Parameters);

	/**
	 * Starts a widget's looping sound for an event (e.g. UISound.Event.Hold), found the same way as Play Widget UI Sound.
	 * Unlike one-shots it plays through an audio component, so it can be stopped later. The sound asset itself must loop.
	 *
	 * @param Widget    The widget being held. Defaults to Self in Blueprint.
	 * @param EventTag  Usually UISound.Event.Hold.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles", meta = (DefaultToSelf = "Widget"))
	void StartUILoop(const UWidget* Widget, FGameplayTag EventTag);

	/**
	 * Like Start UI Loop, but the loop starts with these parameters already set (no jump from the defaults).
	 *
	 * @param Widget      The widget being held. Defaults to Self in Blueprint.
	 * @param EventTag    Usually UISound.Event.Hold.
	 * @param Parameters  e.g. [Value = 0.8]. Update them later with Set UI Loop Parameters.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles", meta = (DefaultToSelf = "Widget"))
	void StartUILoopWithParameters(const UWidget* Widget, FGameplayTag EventTag, const TArray<FAudioParameter>& Parameters);

	/** Stops the widget's loop (short fade, no click). Safe to call if it has none. */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles", meta = (DefaultToSelf = "Widget"))
	void StopUILoop(const UWidget* Widget);

	/** Stops every UI loop immediately. Called automatically when a level starts loading. */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles")
	void StopAllUILoops();

	/**
	 * Sends game data to a widget's playing loop: every parameter in one go (they land together).
	 * Names must match the MetaSound's inputs exactly; unknown names are silently ignored.
	 * Cheap enough to call every frame. Does nothing if the widget has no loop playing.
	 *
	 * @param Widget      The widget whose loop to update. Defaults to Self in Blueprint.
	 * @param Parameters  e.g. [Score = 0.7, Combo = 0.3]. Make each with a Make AudioParameter node.
	 */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles", meta = (DefaultToSelf = "Widget"))
	void SetUILoopParameters(const UWidget* Widget, const TArray<FAudioParameter>& Parameters);

	/**
	 * Replaces the style table used for every lookup.
	 * Used by automated tests to supply their own table; a game could also use it to swap UI sound themes at runtime.
	 *
	 * @param InTable  The new table. nullptr means no table: every lookup finds nothing.
	 */
	void SetStyleTable(USoundStyleTable* InTable);

	/** True for a short time after an Open sound: focus changes then are from code, not the player. */
	bool IsInOpenSettleWindow() const;

	/** True while SoundStyles.Debug is 1. Lets other Sound Styles code (e.g. the table) log only when debugging. */
	static bool IsDebugEnabled();

	/** Call when a screen opens: activates this state's While Open Mix (e.g. UISound.Class.Pause ducks the world). States stack. */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles")
	void PushUIAudioState(UPARAM(meta = (Categories = "UISound.Class")) FGameplayTag StateClass);

	/** Call when the screen closes, with the same state it pushed. */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles")
	void PopUIAudioState(UPARAM(meta = (Categories = "UISound.Class")) FGameplayTag StateClass);

	/** Deactivates every UI audio state. Called automatically when a level starts loading. */
	UFUNCTION(BlueprintCallable, Category = "Sound Styles")
	void ClearAllUIAudioStates();

	/** How many active states are using this mix right now (0 = inactive). For tests and debugging. */
	int32 GetMixCount(const USoundControlBusMix* Mix) const;

private:
	/** Is this widget a scope (implements the scope interface)? If so, OutClasses = its classes. */
	static bool GetScopeClasses(const UObject* Widget, FGameplayTagContainer& OutClasses);

	/**
	 * Shared by the audio-component paths (loops, one-shots with parameters): drops sounds while loading, resolves, logs (debug),
	 * and creates the component. Returns it NOT playing yet (so callers can set parameters first), or nullptr if nothing should play.
	 *
	 * @param bPersistAcrossLevelTransition  True for one-shots, so a sound that starts a level load isn't cut off. False for loops.
	 */
	UAudioComponent* CreateWidgetSound(const UWidget* Widget, FGameplayTag EventTag, const FString& DebugLabel, bool bPersistAcrossLevelTransition);

	/** The widget directly above this one, crossing into the owning Widget Blueprint at the top of a tree. nullptr at the very top. */
	static const UWidget* GetParentWidget(const UWidget* Widget);

	/** Builds the Levels list for Resolve: [0] = OwnClasses, then each scope above the widget, nearest first. */
	static TArray<FGameplayTagContainer> BuildLevels(const UWidget* Widget, const FGameplayTagContainer& OwnClasses);

	/** The full Levels list for a widget: its own classes (a Sound Style widget's Sound Style property, or its scope's), then every scope above it. */
	static TArray<FGameplayTagContainer> GetWidgetLevels(const UWidget* Widget);

	/** The audio component of this widget's playing loop, or nullptr. */
	UAudioComponent* FindUILoop(const UWidget* Widget) const;

	/** Fades out and forgets loops whose widget no longer exists (safety net; widgets normally stop their own loop). */
	void RemoveOrphanedLoops();

	/** Shared gate for every play path: false while a level loads (or just loaded). Starts the Open settle window on Open. */
	bool CanPlayEvent(FGameplayTag EventTag);

	/** With SoundStyles.Debug 1: logs the parameters sent, e.g. "parameters from table: Value=1.00". Does nothing for an empty list. */
	static void LogParameters(const FString& Source, const TArray<FAudioParameter>& Parameters);

	/** Shared by Play UI Sound and Play Widget UI Sound: gate, resolve, log (debug), then play (Slate, or a component if the table has parameters). */
	void PlayLevels(const TArray<FGameplayTagContainer>& Levels, FGameplayTag EventTag, const FString& DebugName);

	/** Adds one user of this mix. Activates it on the first (0 -> 1). */
	void PushMix(USoundControlBusMix* Mix);

	/** Removes one user of this mix. Deactivates it on the last (1 -> 0). Does nothing if it wasn't pushed. */
	void PopMix(USoundControlBusMix* Mix);

	/** The WhileOpenMix on this exact class's row, or nullptr (warns if missing). */
	USoundControlBusMix* FindStateMix(FGameplayTag StateClass) const;

	/** A level is about to load: clear all states and start dropping sounds. */
	void HandlePreLoadMap(const FString& MapName);

	/** The level finished loading: stop dropping sounds after a short settle window. */
	void HandlePostLoadMap(UWorld* LoadedWorld);

	/** True for a short time after a level loads, while the new UI is still being built. */
	bool IsInLoadSettleWindow() const;

	/** Every UI loop playing right now, one per widget. UPROPERTY so the components stay alive. */
	UPROPERTY()
	TArray<FSoundStyleActiveLoop> ActiveLoops;

	/** The loaded style table. UPROPERTY so the garbage collector keeps it (and all its sounds) loaded for the whole game. */
	UPROPERTY()
	TObjectPtr<USoundStyleTable> StyleTable;

	/** Real time of the last Open event. Starts far in the past so the game doesn't begin inside the window. */
	double LastOpenTime = -1000.0;

	/** Real time the last level finished loading. Starts far in the past, same reason. */
	double LastLoadEndTime = -1000.0;

	/** True between PreLoadMap and PostLoadMap. UI sounds are dropped. */
	bool bLoadingMap = false;

	/** How many open screens want each mix. Activated at 0→1, deactivated at 1→0. */
	UPROPERTY()
	TMap<TObjectPtr<USoundControlBusMix>, int32> ActiveMixCounts;

	/** Tickets from binding the engine's level-load delegates. Used to unbind in Deinitialize. */
	FDelegateHandle PreLoadMapHandle;
	FDelegateHandle PostLoadMapHandle;
};
