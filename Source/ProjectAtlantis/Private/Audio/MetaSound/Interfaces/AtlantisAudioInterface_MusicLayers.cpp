#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_MusicLayers.h"
#include "Metasound.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundTrigger.h"

#define LOCTEXT_NAMESPACE "AtlantisMetaSoundInterfaces"

namespace ProjectAtlantisAudio
{
	#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "Atlantis.Music.Layers"

	namespace MusicLayersInterface
	{
		const FMetasoundFrontendVersion& GetVersion()
		{
			static const FMetasoundFrontendVersion Version = { AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };
			return Version;
		}

		const FMetasoundFrontendVersion FrontendVersion{ AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };

		namespace Inputs
		{	
			const FLazyName ActiveSplit(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("ActiveSplit"));
			const FLazyName PassiveSplit(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("PassiveSplit"));
		}

		Audio::FParameterInterfacePtr CreateInterface()
		{
			struct FInterface : public Audio::FParameterInterface
			{
				FInterface()
					: Audio::FParameterInterface{ FrontendVersion.Name, FrontendVersion.Number.ToInterfaceVersion() }
				{
					using namespace Metasound;
					using namespace Inputs;

					// --- Register Inputs ---
					Inputs.Add(
						{
							LOCTEXT("AtlantisActiveSplit", "Active Split"),
							LOCTEXT("AtlantisActiveSplit_Description", "Set from DA_AtlantisMusicAxisMapping. The Player State value where the music switches from the Ambient layer to the Active layer. Raise it to make Active harder to reach."),
							GetMetasoundDataTypeName<float>(),
							FAudioParameter{ ActiveSplit.Resolve(), 0.55f },
							FText(),
							0
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisPassiveSplit", "Passive Split"),
							LOCTEXT("AtlantisPassiveSplit_Description", "Set from DA_AtlantisMusicAxisMapping.The Tension value where the Passive layer comes in. Raise it to make Passive harder to reach. "),
							GetMetasoundDataTypeName<float>(),
							FAudioParameter{ PassiveSplit.Resolve(), 0.5f },
							FText(),
							1
						});
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

	void RegisterMusicLayersInterface()
	{
		Audio::IAudioParameterInterfaceRegistry& AudioParamRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
		AudioParamRegistry.RegisterInterface(MusicLayersInterface::CreateInterface());
	}
}

#undef LOCTEXT_NAMESPACE
