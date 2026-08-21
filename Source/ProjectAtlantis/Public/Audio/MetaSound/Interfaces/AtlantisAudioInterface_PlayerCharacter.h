// Copyright ASGC. All Rights Reserved.

#pragma once

#include "MetasoundFrontendDocument.h"
#include "IAudioParameterInterfaceRegistry.h"

namespace ProjectAtlantisAudio
{
	//This namespace is the "folder" for everything related to this one interface
	//TODO: !!CHANGE THIS!! Rename this namespace to your interface to help keep things organizeds
	namespace PlayerCharacterInterface
	{
		// Returns the version info (name + number) that identifies this interface to the engine.
		const FMetasoundFrontendVersion& GetVersion();

		// Builds (or returns the cached) interface object that gets registered with MetaSounds.
		Audio::FParameterInterfacePtr CreateInterface();

		// The actual version data, declared here, defined in the .cpp.
		extern const FMetasoundFrontendVersion FrontendVersion;

		// --- Inputs ---
		// Each FLazyName below is the exact pin name that will show up on a MetaSound graph.
		// TODO: !!CHANGE THIS!! to reflect the pins you need
		namespace Inputs
		{
			extern const FLazyName CurrentHp;
			extern const FLazyName CurrentO2;
			extern const FLazyName LastNonHostileInteractionTime;
			extern const FLazyName NumEnemiesInRange;
			extern const FLazyName IsHiding;
			extern const FLazyName IsDetected;
			extern const FLazyName IsCrouching;
		}

		//OPTIONAL: Comment out if not using outputs
		namespace Outputs
		{
			extern const FLazyName MyOutput;
		}
	}

	// Call this once, fron AtlantisInterfaceRegistration.cpp
	// TODO: !!CHANGE THIS!!
	void RegisterPlayerCharacterInterface();
}