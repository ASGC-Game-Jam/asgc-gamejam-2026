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

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_MusicAxes.h"

#include "Metasound.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundTrigger.h"

#define LOCTEXT_NAMESPACE "AtlantisMetaSoundInterfaces"

namespace ProjectAtlantisAudio
{
	// TODO: !!CHANGE THIS!! per interface file.
	#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "Atlantis.Music.Axes"

	namespace MusicAxesInterface
	{
		const FMetasoundFrontendVersion& GetVersion()
		{
			static const FMetasoundFrontendVersion Version = { AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };
			return Version;
		}

		const FMetasoundFrontendVersion FrontendVersion{ AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };

		// TODO: !!CHANGE THIS!! to match your header input variables
		namespace Inputs
		{	
			const FLazyName PlayerStateAxis(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("PlayerState"));
			const FLazyName TensionAxis(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Tension"));
			const FLazyName EnvironmentAxis(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("Environment"));

		}

		// TODO: !!CHANGE THIS!! to match your header output variables (or comment out if unused)
		// namespace Outputs
		// {
		// 	const FLazyName MyOutput = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("MyOutput");
		// }

		Audio::FParameterInterfacePtr CreateInterface()
		{
			struct FInterface : public Audio::FParameterInterface
			{
				FInterface()
					: Audio::FParameterInterface{ FrontendVersion.Name, FrontendVersion.Number.ToInterfaceVersion() }
				{
					using namespace Metasound;
					using namespace Inputs;
					//TODO: !!CHANGE THIS!! to enable output use
					//using namespace Outputs; 

					// --- Register Inputs ---
					Inputs.Add(
						{
							LOCTEXT("AtlantisPlayerState", "PlayerState"),
							LOCTEXT("AtlantisPlayerState_Description", "Music intensity from 0 to 1. 0 is the ambient floor (never silent), the middle is passive, 1 is full combat."),
							GetMetasoundDataTypeName<float>(),
							PlayerStateAxis.Resolve(),
							FText(),
							0
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisTension", "Tension"),
							LOCTEXT("AtlantisTension_Description", "Pressure and focus from 0 to 1. Rising values add puzzle, stealth or combat push, and danger subtracts from the mix."),
							GetMetasoundDataTypeName<float>(),
							TensionAxis.Resolve(),
							FText(),
							1
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisEnvironment", "Environment"),
							LOCTEXT("AtlantisEnvironment_Description", "Space type from 0 to 1. Shapes reverb, filtering and compression to match the room, such as underwater, open or close."),
							GetMetasoundDataTypeName<float>(),
							EnvironmentAxis.Resolve(),
							FText(),
							2
						});
					

					// --- Register Outputs ---
					// Outputs.Add(
					// 	{
					// 		LOCTEXT("AtlantisMyOutput", "MyOutput"),
					// 		LOCTEXT("AtlantisIsInvestigationReady_Description", "Is Player Character Investigation Ready?"),
					// 		GetMetasoundDataTypeName<bool>(),
					// 		MyOutput,
					// 		FText(),
					// 		EAudioParameterType::Boolean,
					// 		0
					// 	});
				}
			};

			static Audio::FParameterInterfacePtr InterfacePtr;
			if (InterfacePtr.IsValid() == false)
			{
				InterfacePtr = MakeShared<FInterface>();
			}

			return InterfacePtr;
		}
	}

#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE

	void RegisterMusicAxesInterface()
	{
		Audio::IAudioParameterInterfaceRegistry& AudioParamRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
		AudioParamRegistry.RegisterInterface(MusicAxesInterface::CreateInterface());
	}
}

#undef LOCTEXT_NAMESPACE
