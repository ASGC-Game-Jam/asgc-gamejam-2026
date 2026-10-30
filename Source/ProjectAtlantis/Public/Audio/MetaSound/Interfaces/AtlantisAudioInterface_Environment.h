// Atlantis.Environment: discrete room and surface IDs for MetaSounds, delivered by parameter pack (#105).

#pragma once

#include "CoreMinimal.h"
#include "MetasoundFrontendDocument.h"
#include "IAudioParameterInterfaceRegistry.h"

namespace ProjectAtlantisAudio
{
	namespace EnvironmentInterface
	{
		// Returns the version info (name + number) that identifies this interface to the engine.
		const FMetasoundFrontendVersion& GetVersion();

		// Builds (or returns the cached) interface object that gets registered with MetaSounds.
		Audio::FParameterInterfacePtr CreateInterface();

		// The actual version data, declared here, defined in the .cpp.
		extern const FMetasoundFrontendVersion FrontendVersion;

		// --- Inputs ---
		namespace Inputs
		{
			extern const FLazyName RoomType;
			extern const FLazyName MaterialType;
		}

	}

	void RegisterEnvironmentInterface();
}
