// Copyright ASGC. All Rights Reserved.

#include "Audio/MetaSound/Interfaces/AtlantisAudioInterface_PlayerCharacter.h"

#include "Metasound.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundTrigger.h"

// LOCTEXT_NAMESPACE just groups all the localizable text below under one label for the localization system.
#define LOCTEXT_NAMESPACE "AtlantisMetaSoundInterfaces"

namespace ProjectAtlantisAudio
{
	// This define is the "namespace string" for the interface, it becomes part of the interface's
	// full registered name, e.g. "Atlantis.Character". 
	// TODO: !!CHANGE THIS!! per interface file.
	#define AUDIO_PARAMETER_INTERFACE_NAMESPACE "Atlantis.Character"

	namespace PlayerCharacterInterface
	{

		// GetVersion() hands back a static, lazily-built version struct.
		// "static" here means it's built once and reused, not rebuilt every call.
		const FMetasoundFrontendVersion& GetVersion()
		{
			static const FMetasoundFrontendVersion Version = { AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };
			return Version;
		}

		// This is the "real" copy of the version data that the header just declared as extern.
		const FMetasoundFrontendVersion FrontendVersion{ AUDIO_PARAMETER_INTERFACE_NAMESPACE, { 1, 0 } };

		// TODO: !!CHANGE THIS!! pins for this system's actual data
		// AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE builds the full pin name using the namespace above,
		// so "Health" becomes something like "ProjectAtlantis.PlayerCharacter.CurrentHp" internally.
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

		//OPTIONAL: Comment out if not using outputs
		namespace Outputs
		{
			const FLazyName MyOutput = AUDIO_PARAMETER_INTERFACE_MEMBER_DEFINE("MyOutput");
		}

		// CreateInterface() builds the actual interface object: a list of input pins and output pins.
		// This is what gets handed to the registry so MetaSound graphs can "implement" it.
		Audio::FParameterInterfacePtr CreateInterface()
		{
			// FInterface is a small local struct, it only exists to build itself once in its constructor.
			struct FInterface : public Audio::FParameterInterface
			{
				FInterface()
					// This base constructor call registers the interface's name + version number.
					: Audio::FParameterInterface{ FrontendVersion.Name, FrontendVersion.Number.ToInterfaceVersion() }
				{
					using namespace Metasound;
					using namespace Inputs;

					//OPTIONAL: Comment out if not using outputs
					using namespace Outputs; 
					

					// TODO: !!CHANGE THIS!! one Inputs.Add(...) block per input pin.
					// Each entry is: display name, tooltip/description, data type, actual pin name,
					// an optional default value text, then a sort index (controls pin order in editor).
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
							GetMetasoundDataTypeName<int>(),
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


					// OPTIONAL! Comment out if not using outputs
					Outputs.Add(
						{
							LOCTEXT("AtlantisMyOutput", "MyOutput"),// Name to be displayed in editor or tools
							LOCTEXT("AtlantisIsInvestigationReady_Description", "Is Player Character Investigation Ready?"), // Description to be displayed in editor or tools
							GetMetasoundDataTypeName<bool>(), // FName describing the type of the data.
							MyOutput, // Specified Pin
							FText(), // Text to display in the editor or tools if the consuming system of the given input parameter is not implemented
							EAudioParameterType::Boolean, // Type of output parameter used as a runtime identifier if unspecified by the DataType.
							0 // Sort Order
						});
				}
			};

			// Only build FInterface once, then reuse the same pointer every time CreateInterface() is called.
			static Audio::FParameterInterfacePtr InterfacePtr;

			if (InterfacePtr.IsValid() == false)
			{
				InterfacePtr = MakeShared<FInterface>();
			}

			return InterfacePtr;
		}
	}

	// Undo the define so it doesn't leak into other files that get compiled after this one.
#undef AUDIO_PARAMETER_INTERFACE_NAMESPACE

	// This is the function your module calls on startup, it's the "switch" that actually
	// makes the interface show up in the MetaSound editor's Interfaces panel.
	// TODO !! CHANGE THIS !! - Must match your .h and be different for each interface
	void ProjectAtlantisAudio::RegisterPlayerCharacterInterface()
	{
		Audio::IAudioParameterInterfaceRegistry& AudioParamRegistry = Audio::IAudioParameterInterfaceRegistry::Get();
		
		//TODO: !!CHANGE THIS!! - Ensure the interface namespace is the same as your .h
		AudioParamRegistry.RegisterInterface(PlayerCharacterInterface::CreateInterface());
	}
}

#undef LOCTEXT_NAMESPACE