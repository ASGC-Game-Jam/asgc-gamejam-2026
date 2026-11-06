// Copyright (c) 2026 ASGC

#include "Audio/AtlantisAudioFactTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioFact_Oxygen_Level, "Audio.Fact.Oxygen.Level", "FloatPayload: available oxygen as a 0-1 fraction of capacity.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioFact_Oxygen_Critical, "Audio.Fact.Oxygen.Critical", "BoolPayload: true while oxygen is critical.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioFact_Stealth_Hiding, "Audio.Fact.Stealth.Hiding", "BoolPayload: true while the player is in a hiding spot.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioFact_Investigation_Status, "Audio.Fact.Investigation.Status", "KeyedStatePayload: Key is the investigation title, State is an Audio.State.Investigation tag.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioFact_Enemy_Status, "Audio.Fact.Enemy.Status", "KeyedStatePayload: Key is the enemy's name, State is an Audio.State.Enemy tag.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Enemy_None, "Audio.State.Enemy.None", "Mirrors EMonsterStatus::None. Enemy is hidden or gone; send this on EndPlay so the music forgets it.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Enemy_Idle, "Audio.State.Enemy.Idle", "Mirrors EMonsterStatus::Idle.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Enemy_Alerted, "Audio.State.Enemy.Alerted", "Mirrors EMonsterStatus::Alerted.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Enemy_Searching, "Audio.State.Enemy.Searching", "Mirrors EMonsterStatus::Searching.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Enemy_Combat, "Audio.State.Enemy.Combat", "Mirrors EMonsterStatus::Combat.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Investigation_Undiscovered, "Audio.State.Investigation.Undiscovered", "Mirrors EJournalInvestigationStatus::Undiscovered.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Investigation_InProgress, "Audio.State.Investigation.InProgress", "Mirrors EJournalInvestigationStatus::Inprogress.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioState_Investigation_Complete, "Audio.State.Investigation.Complete", "Mirrors EJournalInvestigationStatus::Complete.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Room_Default, "Room.Default", "The player isn't inside any room volume. Sounds neutral.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_AudioFact_Environment_Room, "Audio.Fact.Environment.Room", "TagPayload: the Room tag for the space the player is in.");
