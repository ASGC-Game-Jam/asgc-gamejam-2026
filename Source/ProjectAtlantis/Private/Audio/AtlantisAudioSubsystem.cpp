// Fill out your copyright notice in the Description page of Project Settings.

#include "Audio/AtlantisAudioSubsystem.h"

#include "Audio/AtlantisAudioDeveloperSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioSubsystem, Log, All);

void UAtlantisAudioSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
	Super::Initialize(Collection);

	bIsActive = UAtlantisAudioDeveloperSettings::DetermineIfMusicSubsystemEnabled();
	UE_LOG(LogAtlantisAudioSubsystem, Log, TEXT("Atlantis Music Subsystem Status: %s") , bIsActive? TEXT("Active") : TEXT("Bypassed"));
}

void UAtlantisAudioSubsystem::Deinitialize()
{
	Super::Deinitialize();
}
