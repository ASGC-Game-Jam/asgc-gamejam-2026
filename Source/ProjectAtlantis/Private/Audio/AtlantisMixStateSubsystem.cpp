// Copyright (c) 2026 ASGC

#include "Audio/AtlantisMixStateSubsystem.h"

#include "Audio/PubSubSystem/ASGC_DataRouterSubsystem.h"
#include "Audio/AtlantisAudioDeveloperSettings.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "AudioModulationStatics.h"
#include "SoundControlBusMix.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisMixState, Log, All);

void UAtlantisMixStateSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	UASGC_DataRouterSubsystem* Router = InWorld.GetSubsystem<UASGC_DataRouterSubsystem>();

	if (!Router)
	{
		UE_LOG(LogAtlantisMixState, Error, TEXT("No Data Router in world [%s]. Mixes won't apply to sounds."), *InWorld.GetName());
		return;
	}

	for (const FAtlantisMixStateRule& Rule : UAtlantisAudioDeveloperSettings::GetMixStateRules())
	{
		// Two rules can share a fact, but one subscription delivers it to both.
		if (!Rule.WhileFact.IsValid() || FactSubscriptions.Contains(Rule.WhileFact))
		{
			continue;
		}

		const int32 Handle = Router->Subscribe(Rule.WhileFact, this);
		if (Handle != INDEX_NONE)
		{
			FactSubscriptions.Add(Rule.WhileFact, Handle);
		}
	}
}

void UAtlantisMixStateSubsystem::PreDeinitialize()
{
	if (UWorld* World = GetWorld())
	{
		if (UASGC_DataRouterSubsystem* Router = World->GetSubsystem<UASGC_DataRouterSubsystem>())
		{
			for (const auto& Pair : FactSubscriptions)
			{
				Router->Unsubscribe(Pair.Key, Pair.Value);
			}
		}
	}

	FactSubscriptions.Reset();

	for (USoundControlBusMix* Mix : ActiveMixes)
	{
		if (IsValid(Mix))
		{
			UAudioModulationStatics::DeactivateBusMix(this, Mix);
		}
	}
	ActiveMixes.Reset();

	Super::PreDeinitialize();
}

void UAtlantisMixStateSubsystem::OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload)
{
	// Only bool facts drive mixes for now.
	const FAtlantisAudioBoolPayload* BoolFactPtr = Payload.GetPtr<FAtlantisAudioBoolPayload>();
	if (!BoolFactPtr)
	{
		return;
	}

	for (const FAtlantisMixStateRule& Rule : UAtlantisAudioDeveloperSettings::GetMixStateRules())
	{
		if (Rule.WhileFact != Tag)
		{
			continue;
		}

		// Loads the mix the first time. After that it's already in memory, so this is quick.
		USoundControlBusMix* Mix = Rule.Mix.LoadSynchronous();
		if (!IsValid(Mix))
		{
			continue;
		}

		// The fade in and out comes from the mix's own attack and release times.
		if (BoolFactPtr->bValue)
		{
			UAudioModulationStatics::ActivateBusMix(this, Mix);
			ActiveMixes.Add(Mix);
		}
		else
		{
			UAudioModulationStatics::DeactivateBusMix(this, Mix);
			ActiveMixes.Remove(Mix);
		}
	}
}

bool UAtlantisMixStateSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}