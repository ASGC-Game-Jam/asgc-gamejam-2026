// Copyright (c) 2026 ASGC

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_Environment.h"
#include "Metasound.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundTrigger.h"

#define LOCTEXT_NAMESPACE "AtlantisMetaSoundInterfaces"

namespace ProjectAtlantisAudio
{
	#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "Atlantis.Environment"

	namespace EnvironmentInterface
	{
		const FMetasoundFrontendVersion& GetVersion()
		{
			static const FMetasoundFrontendVersion Version = { AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };
			return Version;
		}

		const FMetasoundFrontendVersion FrontendVersion{ AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };

		namespace Inputs
		{	
			const FLazyName RoomType(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("RoomType"));
			const FLazyName MaterialType(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("MaterialType"));
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
							LOCTEXT("AtlantisRoomType", "RoomType"),
							LOCTEXT("AtlantisRoomType_Description", "Kind of space the player is in, set by room volumes. IDs come from DA_AtlantisRoomTypeTable."),
							GetMetasoundDataTypeName<int32>(),
							RoomType.Resolve(),
							FText(),
							0
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisMaterialType", "MaterialType"),
							LOCTEXT("AtlantisMaterialType_Description", "Surface under the player, from a physical material trace. IDs match Project Settings > Physics > Physical Surface (0 = Default)."),
							GetMetasoundDataTypeName<int32>(),
							MaterialType.Resolve(),
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

	void RegisterEnvironmentInterface()
	{
		Audio::IAudioParameterInterfaceRegistry& AudioParamRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
		AudioParamRegistry.RegisterInterface(EnvironmentInterface::CreateInterface());
	}
}

#undef LOCTEXT_NAMESPACE
