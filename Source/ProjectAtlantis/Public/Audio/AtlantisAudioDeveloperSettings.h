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
 struct FInstancedStruct;

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
	static TObjectPtr<UMetaSoundSource> GetMusicMetaSoundSource(); 
	
	/** Checks Tag is registered and Payload holds the registered struct type. Fails loudly on a breach. Returns true if valid. */
	static bool ValidatePayload(FGameplayTag Tag, const FInstancedStruct& Payload);

protected:
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Subsystem Settings")
	bool bEnableMusicManagementSubsystem = true;

	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Subsystem Settings")
	TSoftObjectPtr<UMetaSoundSource> MusicMetaSoundSource;

	/** Maps each audio fact tag to the struct type its payload must use. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Validation")
	TMap<FGameplayTag, TObjectPtr<const UScriptStruct>> PayloadRegistry;

};
