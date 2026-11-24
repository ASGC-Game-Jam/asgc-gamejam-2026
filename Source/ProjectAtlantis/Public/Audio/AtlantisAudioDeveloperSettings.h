// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Audio/AtlantisMixStateTypes.h"

#include "AtlantisAudioDeveloperSettings.generated.h"

 class UMetaSoundSource;
 class UAtlantisMusicAxisMapping;
 struct FInstancedStruct;
 class UAtlantisRoomTypeTable;
 class UDataTable;

/** Project-wide audio settings, under Project Settings > Game > Atlantis Audio Settings. */
UCLASS(config=Game, DefaultConfig)
class PROJECTATLANTIS_API UAtlantisAudioDeveloperSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	virtual FName GetCategoryName() const override;
#if WITH_EDITOR
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
#endif

	/** True if the music subsystem should run. The atlantis.audio.musicSubsystem console variable can force it on or off. */
	static bool DetermineIfMusicSubsystemEnabled();

	/** The score for this world's map, from Music Per Map. nullptr if this world shouldn't play music. */
	static UMetaSoundSource* GetMusicForWorld(const UWorld* World);

	static bool IsMusicPlaybackWorld(const UWorld* World);
	
	/** True if Tag is registered and Payload holds its registered struct. Logs an error for any mismatch. */
	static bool ValidatePayload(FGameplayTag Tag, const FInstancedStruct& Payload);

	/** Loads the music axis mapping asset. Returns null if none is set. */
	static UAtlantisMusicAxisMapping* GetMusicAxisMapping();

	/** Finds the RoomType pin ID for a room tag. Logs an error and returns false if the tag or table is missing. */
	static bool GetRoomTypeId(FGameplayTag RoomTag, int32& OutRoomTypeId);

	/** Loads the player audio table. Returns null if none is set. */
	static UDataTable* GetPlayerAudioTable();

	/** The Mix States list. Empty if none are set. */
	static const TArray<FAtlantisMixStateRule>& GetMixStateRules();

protected:

	friend struct FAtlantisAudioValidationTestAccess;
	
	/** Turns the music subsystem on or off for the whole project. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Subsystem Settings")
	bool bEnableMusicManagementSubsystem = true;

	/** Each gameplay map and the score it plays. Maps not listed play no subsystem music. Menu music is owned by the menu Blueprint. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Subsystem Settings", meta=(DisplayName="Music Per Map"))
	TMap<TSoftObjectPtr<UWorld>, TSoftObjectPtr<UMetaSoundSource>> MusicPerMap;

	/** Maps each audio fact tag to the struct type its payload must use. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Validation")
	TMap<FGameplayTag, TSoftObjectPtr<const UScriptStruct>> PayloadRegistry;

	/** The routing sheet the music subsystem uses to turn facts into axis values. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Music Axes")
	TSoftObjectPtr<UAtlantisMusicAxisMapping> MusicAxisMapping;

	/** The table that matches room tags to RoomType pin IDs. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Rooms")
	TSoftObjectPtr<UAtlantisRoomTypeTable> RoomTypeTable;

	/** The table of suit sounds the player audio subsystem plays, and the facts that drive them. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Player Audio", meta = (RequiredAssetDataTags = "RowStructure=/Script/ProjectAtlantis.AtlantisPlayerAudioRow"))
	TSoftObjectPtr<UDataTable> PlayerAudioData;

	/** Each bus mix the mix state subsystem turns on, and the bool fact that turns it on. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Mix States")
	TArray<FAtlantisMixStateRule> MixStates;

private:
	/** This world's entry in Music Per Map, or nullptr if its map isn't listed. Handles PIE map names. */
	static const TSoftObjectPtr<UMetaSoundSource>* FindMusicEntry(const UWorld* World);
};
