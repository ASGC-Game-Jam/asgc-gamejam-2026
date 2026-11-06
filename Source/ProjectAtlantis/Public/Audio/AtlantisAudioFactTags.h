// Copyright (c) 2026 ASGC

#pragma once

#include "NativeGameplayTags.h"

// Fact tags the audio ecosystem understands. Register each one's payload struct in Project Settings > Atlantis Audio Validation.
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioFact_Oxygen_Level);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioFact_Oxygen_Critical);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioFact_Stealth_Hiding);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioFact_Investigation_Status);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioFact_Enemy_Status);

// State tags carried in FAtlantisAudioKeyedStatePayload::State. Each mirrors a gameplay enum value one to one.
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Enemy_None);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Enemy_Idle);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Enemy_Alerted);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Enemy_Searching);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Enemy_Combat);

PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Investigation_Undiscovered);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Investigation_InProgress);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioState_Investigation_Complete);

// The room reported when the player isn't inside any room volume. Other Room tags live in DefaultGameplayTags.ini.
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Room_Default);
PROJECTATLANTIS_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AudioFact_Environment_Room);
