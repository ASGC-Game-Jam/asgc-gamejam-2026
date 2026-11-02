// Atlantis.Environment: discrete room and surface IDs for MetaSounds.

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

		// The actual version data.
		extern const FMetasoundFrontendVersion FrontendVersion;

		namespace Inputs
		{
			extern const FLazyName RoomType;
			extern const FLazyName MaterialType;
		}

	}

	void RegisterEnvironmentInterface();
}
