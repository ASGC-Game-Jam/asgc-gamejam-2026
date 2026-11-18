// Copyright (c) 2026 ASGC
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterfaceRegistration.h"

// Include each new interface's header here.
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_MusicAxes.h"
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_Environment.h"
#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_MusicLayers.h"

namespace ProjectAtlantisAudio
{
	void RegisterAllMetaSoundInterfaces()
	{
		// Call each new interface's Register function here.
		RegisterMusicAxesInterface();
		RegisterEnvironmentInterface();
		RegisterMusicLayersInterface();
	}
}