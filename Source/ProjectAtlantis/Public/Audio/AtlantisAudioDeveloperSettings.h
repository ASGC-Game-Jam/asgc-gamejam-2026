// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "AtlantisAudioDeveloperSettings.generated.h"

/**
 * Stores any audio dev settings
 */

 class UMetaSoundSource;
 class UAtlantisMusicAxisMapping;
 struct FInstancedStruct;
 class UAtlantisRoomTypeTable;


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

	static bool DetermineIfMusicSubsystemEnabled();

	/** The score for this world's map, from Music Per Map. nullptr if this world shouldn't play music. */
	static UMetaSoundSource* GetMusicForWorld(const UWorld* World);

	static bool IsMusicPlaybackWorld(const UWorld* World);
	
	/** Checks Tag is registered and Payload holds the registered struct type. Fails loudly on a breach. Returns true if valid. */
	static bool ValidatePayload(FGameplayTag Tag, const FInstancedStruct& Payload);

	static UAtlantisMusicAxisMapping* GetMusicAxisMapping();

	/** Finds the RoomType pin ID for a room tag. Logs an error and returns false if the tag or table is missing. */
	static bool GetRoomTypeId(FGameplayTag RoomTag, int32& OutRoomTypeId);


protected:

	friend struct FAtlantisAudioValidationTestAccess;
	
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

private:
	/** This world's entry in Music Per Map, or nullptr if its map isn't listed. Handles PIE map names. */
	static const TSoftObjectPtr<UMetaSoundSource>* FindMusicEntry(const UWorld* World);
};
