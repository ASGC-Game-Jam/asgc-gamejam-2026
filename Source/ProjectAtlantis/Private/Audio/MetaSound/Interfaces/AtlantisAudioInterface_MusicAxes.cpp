// Atlantis.Music.Axes: the music's PlayerState, Tension and Environment axes. Also the template for new interfaces.
// To make a new interface, copy this pair and follow the !!CHANGE THIS!! markers, as README.md explains.

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
							LOCTEXT("AtlantisPlayerState_Description", "Music intensity from 0 to 1. Currently unused, so it stays at its baseline."),
							GetMetasoundDataTypeName<float>(),
							PlayerStateAxis.Resolve(),
							FText(),
							0
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisTension", "Tension"),
							LOCTEXT("AtlantisTension_Description", "Pressure from 0 to 1, raised by states such as low oxygen or hiding. Higher values bring in the Passive layer."),
							GetMetasoundDataTypeName<float>(),
							TensionAxis.Resolve(),
							FText(),
							1
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisEnvironment", "Environment"),
							LOCTEXT("AtlantisEnvironment_Description", "Space type from 0 to 1. Shapes reverb, filtering and compression to match the room."),
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
