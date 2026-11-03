// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "AtlantisAudioDeveloperSettings.generated.h"


/** Music axes that gameplay facts can drive. Environment isn't here because room volumes will set it. */
UENUM()
enum class EAtlantisMusicAxis : uint8
{
	PlayerState,
	Tension
};

/** One routing row: when this tag is active, it pushes this axis by this amount. */
USTRUCT()
struct FAtlantisMusicAxisRule
{
	GENERATED_BODY()

	/** A fact tag for Float and Bool facts, or an Audio.State tag for keyed facts like enemies. */
	UPROPERTY(EditAnywhere, Category = "Atlantis Audio", meta = (Categories = "Audio"))
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, Category = "Atlantis Audio")
	EAtlantisMusicAxis Axis = EAtlantisMusicAxis::Tension;

	/** How far this rule pushes the axis. Negative values pull it down. */
	UPROPERTY(EditAnywhere, Category = "Atlantis Audio")
	float Amount = 0.f;

	/** Float facts only: use 1 - Value, so a lower number has a stronger effect (e.g. low oxygen). */
	UPROPERTY(EditAnywhere, Category = "Atlantis Audio")
	bool bInvertValue = false;
};


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

	static const TArray<FAtlantisMusicAxisRule>& GetMusicAxisRules();
	static float GetAxisBaseline(EAtlantisMusicAxis Axis);

protected:

	friend struct FAtlantisAudioValidationTestAccess;
	
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Subsystem Settings")
	bool bEnableMusicManagementSubsystem = true;

	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Subsystem Settings")
	TSoftObjectPtr<UMetaSoundSource> MusicMetaSoundSource;

	/** Maps each audio fact tag to the struct type its payload must use. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Audio Validation")
	TMap<FGameplayTag, TSoftObjectPtr<const UScriptStruct>> PayloadRegistry;

	/** Where each axis sits when nothing is pushing it. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Music Axes", meta = (ClampMin = "0", ClampMax = "1"))
	float PlayerStateBaseline = 0.5f;

	UPROPERTY(Config, EditAnywhere, Category="Atlantis Music Axes", meta = (ClampMin = "0", ClampMax = "1"))
	float TensionBaseline = 0.3f;

	/** Routes facts to music axes. */
	UPROPERTY(Config, EditAnywhere, Category="Atlantis Music Axes")
	TArray<FAtlantisMusicAxisRule> MusicAxisRules;

};
