// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AtlantisAudioSubsystem.generated.h"



DECLARE_LOG_CATEGORY_EXTERN(AtlantisAudioSubsystemLogCategry, Log, All);

/**
 *  Manages the Music Subsystem for Project Atlantis
 */

UCLASS()
class PROJECTATLANTIS_API UAtlantisAudioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

private:
	bool bIsActive = true;
	
};
