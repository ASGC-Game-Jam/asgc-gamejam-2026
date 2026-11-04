// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AtlantisMusicAxisMapping.generated.h"

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


/** The music's routing sheet: where each axis rests, and how gameplay facts push it. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisMusicAxisMapping : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Where PlayerState sits when nothing is pushing it. */
	UPROPERTY(EditAnywhere, Category = "Music Axes", meta = (ClampMin = "0", ClampMax = "1"))
	float PlayerStateBaseline = 0.5f;

	/** Where Tension sits when nothing is pushing it. */
	UPROPERTY(EditAnywhere, Category = "Music Axes", meta = (ClampMin = "0", ClampMax = "1"))
	float TensionBaseline = 0.3f;

	UPROPERTY(EditAnywhere, Category = "Music Axes")
	TArray<FAtlantisMusicAxisRule> Rules;
};
