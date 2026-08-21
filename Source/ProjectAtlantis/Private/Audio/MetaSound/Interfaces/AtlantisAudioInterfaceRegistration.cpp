// Copyright Your Studio. All Rights Reserved.
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.h"

// TODO: include each new interface header here as you add them
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_PlayerCharacter.h"


namespace ProjectAtlantisAudio
{
	void RegisterAllMetaSoundInterfaces()
	{
		// TODO: call each new interface's Register function here
		RegisterPlayerCharacterInterface();

	}
}