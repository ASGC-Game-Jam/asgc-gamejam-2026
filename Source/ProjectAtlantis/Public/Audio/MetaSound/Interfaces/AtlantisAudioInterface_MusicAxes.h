// Atlantis.Music.Axes: the music's PlayerState, Tension and Environment axes. Also the template for new interfaces.
// To make a new interface, copy this pair and follow the !!CHANGE THIS!! markers, as README.md explains.

#pragma once

#include "CoreMinimal.h"
#include "MetasoundFrontendDocument.h"
#include "IAudioParameterInterfaceRegistry.h"

namespace ProjectAtlantisAudio
{
	// TODO: !!CHANGE THIS!! Rename this namespace to your interface.
	namespace MusicAxesInterface
	{
		// Returns the version info (name + number) that identifies this interface to the engine.
		const FMetasoundFrontendVersion& GetVersion();

		// Builds (or returns the cached) interface object that gets registered with MetaSounds.
		Audio::FParameterInterfacePtr CreateInterface();

		// The version data, defined in the .cpp.
		extern const FMetasoundFrontendVersion FrontendVersion;

		// --- Inputs ---
		// TODO: !!CHANGE THIS!! to reflect the input pins you need
		namespace Inputs
		{
			extern const FLazyName PlayerStateAxis;
			extern const FLazyName TensionAxis;
			extern const FLazyName EnvironmentAxis;
		}

		// --- Outputs ---
		// TODO: !!CHANGE THIS!! to reflect the output pins you need (or comment out if unused)
		// namespace Outputs
		// {
		// 	extern const FLazyName MyOutput;
		// }
	}

	// Call this once from AtlantisAudioInterfaceRegistration.cpp.
	// TODO: !!CHANGE THIS!!
	void RegisterMusicAxesInterface();
}
