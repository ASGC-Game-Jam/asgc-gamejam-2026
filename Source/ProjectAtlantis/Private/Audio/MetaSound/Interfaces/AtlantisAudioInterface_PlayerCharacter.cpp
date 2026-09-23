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

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_PlayerCharacter.h"

#include "Metasound.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundTrigger.h"

#define LOCTEXT_NAMESPACE "AtlantisMetaSoundInterfaces"

namespace ProjectAtlantisAudio
{
	// TODO: !!CHANGE THIS!! per interface file.
	#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "Atlantis.Character"

	namespace PlayerCharacterInterface
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
			const FLazyName CurrentHp(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("CurrentHp"));
			const FLazyName CurrentO2(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("CurrentO2"));
			const FLazyName LastNonHostileInteractionTime(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("LastNonHostileInteractionTime"));
			const FLazyName NumEnemiesInRange(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("NumEnemiesInRange"));
			const FLazyName IsHiding(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("IsHiding"));
			const FLazyName IsDetected(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("IsDetected"));
			const FLazyName IsCrouching(AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("IsCrouching"));
		}

		// TODO: !!CHANGE THIS!! to match your header output variables (or comment out if unused)
		namespace Outputs
		{
			const FLazyName MyOutput = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("MyOutput");
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
					using namespace Outputs; 

					// --- Register Inputs ---
					Inputs.Add(
						{
							LOCTEXT("AtlantisPlayerHealth", "Health"),
							LOCTEXT("AtlantisPlayerHealth_Description", "Current player health value."),
							GetMetasoundDataTypeName<float>(),
							CurrentHp.Resolve(),
							FText(),
							0
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisPlayerOxygen", "CurrentOxygen"),
							LOCTEXT("AtlantisPlayerOxygen_Description", "Player's current normalized O2 level"),
							GetMetasoundDataTypeName<float>(),
							CurrentO2.Resolve(),
							FText(),
							1
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisLastNonHostileInteractionTime", "LastNonHostileInteractionTime"),
							LOCTEXT("AtlantisLastNonHostileInteractionTime_Description", "Last Non-Hostile Interaction Time"),
							GetMetasoundDataTypeName<float>(),
							LastNonHostileInteractionTime.Resolve(),
							FText(),
							2
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisNumEnemiesInRange", "NumEnemiesInRange"),
							LOCTEXT("AtlantisNumEnemiesInRange_Description", "Number of Enemies In Range"),
							GetMetasoundDataTypeName<int32>(), // FIXED: Changed int to int32 for MetaSound compatibility
							NumEnemiesInRange.Resolve(),
							FText(),
							3
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisIsHiding", "IsHiding"),
							LOCTEXT("AtlantisIsHiding_Description", "Is Player Character Hiding?"),
							GetMetasoundDataTypeName<bool>(),
							IsHiding.Resolve(),
							FText(),
							4
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisIsDetected", "IsDetected"),
							LOCTEXT("AtlantisIsDetected_Description", "Is Player Character Detected?"),
							GetMetasoundDataTypeName<bool>(),
							IsDetected.Resolve(),
							FText(),
							5
						});
					Inputs.Add(
						{
							LOCTEXT("AtlantisIsCrouching", "IsCrouching"),
							LOCTEXT("AtlantisIsCrouching_Description", "Is Player Character Crouching?"),
							GetMetasoundDataTypeName<bool>(),
							IsCrouching.Resolve(),
							FText(),
							6
						});

					// --- Register Outputs ---
					Outputs.Add(
						{
							LOCTEXT("AtlantisMyOutput", "MyOutput"),
							LOCTEXT("AtlantisIsInvestigationReady_Description", "Is Player Character Investigation Ready?"),
							GetMetasoundDataTypeName<bool>(),
							MyOutput,
							FText(),
							EAudioParameterType::Boolean,
							0
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

	void RegisterPlayerCharacterInterface()
	{
		Audio::IAudioParameterInterfaceRegistry& AudioParamRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
		AudioParamRegistry.RegisterInterface(PlayerCharacterInterface::CreateInterface());
	}
}

#undef LOCTEXT_NAMESPACE
