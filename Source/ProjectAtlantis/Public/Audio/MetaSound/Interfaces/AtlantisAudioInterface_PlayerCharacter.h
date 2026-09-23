// =======================================================================================
// PROJECT ATLANTIS - METASOUND INTERFACE TEMPLATE
// =======================================================================================
// DESIGN NOTE: This file intentionally uses a boilerplate/template structure. 
// Due to Unreal Engine's static registration requirements and dependency on the 
// preprocessor (AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE), traditional C++ abstraction 
// and inheritance cannot be used here.
//
// HOW TO USE: 
// 1. Copy this file pairs (.h/.cpp) to create a new interface.
// 2. Follow the !!CHANGE THIS!! TODO markers to rename namespaces and define your pins.
// =======================================================================================

#pragma once

#include "CoreMinimal.h"
#include "MetasoundFrontendDocument.h"
#include "IAudioParameterInterfaceRegistry.h"

namespace ProjectAtlantisAudio
{
	//TODO: !!CHANGE THIS!! Rename this namespace to your interface to help keep things organized
	namespace PlayerCharacterInterface
	{
		// Returns the version info (name + number) that identifies this interface to the engine.
		const FMetasoundFrontendVersion& GetVersion();

		// Builds (or returns the cached) interface object that gets registered with MetaSounds.
		Audio::FParameterInterfacePtr CreateInterface();

		// The actual version data, declared here, defined in the .cpp.
		extern const FMetasoundFrontendVersion FrontendVersion;

		// --- Inputs ---
		// TODO: !!CHANGE THIS!! to reflect the input pins you need
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

		// --- Outputs ---
		// TODO: !!CHANGE THIS!! to reflect the output pins you need (or comment out if unused)
		namespace Outputs
		{
			extern const FLazyName MyOutput;
		}
	}

	// Call this once from AtlantisInterfaceRegistration.cpp
	// TODO: !!CHANGE THIS!!
	void RegisterPlayerCharacterInterface();
}
