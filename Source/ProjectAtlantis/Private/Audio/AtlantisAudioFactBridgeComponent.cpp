// Copyright (c) 2026 ASGC

#include "Audio/AtlantisAudioFactBridgeComponent.h"

#include "Audio/AtlantisAudioDataBPFL.h"
#include "Audio/AtlantisAudioFactTags.h"
#include "Audio/AtlantisAudioPayloadTypes.h"
#include "Core/AtlantisHidingComponent.h"
#include "Core/AtlantisOxygenComponent.h"
#include "Core/AtlantisPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "StructUtils/InstancedStruct.h"
#include "TimerManager.h"
#include "Core/AtlantisMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PhysicsVolume.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioFactBridge, Log, All);

namespace
{
	// About two seconds at 60 fps.
	constexpr int32 MaxPlayerStateRetries = 120;
}


void UAtlantisAudioFactBridgeComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("Fact bridge on [%s] must be on a pawn. No gameplay facts will reach the music."), *GetNameSafe(GetOwner()));
		return;
	}

	// The pawn can begin play before it's possessed, so try again when its controller changes.
	Pawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::HandleControllerChanged);
	TryBindSources();
}

void UAtlantisAudioFactBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		Pawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged);
	}

	// Also cancels any pending player state retry.
	UnbindSources();

	Super::EndPlay(EndPlayReason);

}

void UAtlantisAudioFactBridgeComponent::TryBindSources()
{
	if (bSourcesBound)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	AAtlantisPlayerState* PlayerState = Pawn->GetPlayerState<AAtlantisPlayerState>();
	if (!PlayerState)
	{
		// The player state can arrive a few ticks after possession. If it never does, the game
		// mode is probably using a different player state class, so stop retrying and say so.
		if (PlayerStateRetryCount >= MaxPlayerStateRetries)
		{
			UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No AAtlantisPlayerState on [%s] after %d ticks (found [%s]). Oxygen and hiding facts won't reach the music."),
				*GetNameSafe(Pawn), MaxPlayerStateRetries, *GetNameSafe(Pawn->GetPlayerState()));
			return;
		}

		++PlayerStateRetryCount;
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TryBindSources);
		return;
	}

	bSourcesBound = true;
	PlayerStateRetryCount = 0;

	// Tell audio listeners a local player is bound, before any other fact arrives.
	FAtlantisAudioBoolPayload Payload;
	Payload.bValue = true;
	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Player_Present, FInstancedStruct::Make(Payload));

	OxygenComponent = PlayerState->GetOxygenComponent();
	if (OxygenComponent)
	{
		OxygenComponent->OnCurrentOxygenChanged.AddDynamic(this, &ThisClass::HandleCurrentOxygenChanged);
		OxygenComponent->OnCriticalOxygenChanged.AddDynamic(this, &ThisClass::HandleCriticalOxygenChanged);
		OxygenComponent->OnAllocatedOxygenChanged.AddDynamic(this, &ThisClass::HandleAllocatedOxygenChanged);
		OxygenComponent->OnOxygenCapacityChanged.AddDynamic(this, &ThisClass::HandleOxygenCapacityChanged);
	}
	else
	{
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No oxygen component on [%s]. Oxygen facts won't reach the music."), *GetNameSafe(PlayerState));
	}

	HidingComponent = Pawn->FindComponentByClass<UAtlantisHidingComponent>();
	if (HidingComponent)
	{
		HidingComponent->OnHidingStateChanged.AddDynamic(this, &ThisClass::HandleHidingStateChanged);
	}
	else
	{
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No hiding component on [%s]. Hiding facts won't reach the music."), *GetNameSafe(Pawn));
	}

	MovementComponent = Cast<UAtlantisMovementComponent>(Pawn->GetMovementComponent());
	if (MovementComponent)
	{
		MovementComponent->OnUnderwaterChanged.AddDynamic(this, &ThisClass::HandleUnderwaterChanged);

		// The listener has no event for moving, so check it on a repeating timer.
		GetWorld()->GetTimerManager().SetTimer(ListenerCheckTimer, this, &ThisClass::HandleListenerCheckTimer, ListenerCheckInterval, true);
	}
	else
	{
		UE_LOG(LogAtlantisAudioFactBridge, Error, TEXT("No UAtlantisMovementComponent on [%s]. Underwater facts won't reach audio."), *GetNameSafe(Pawn));
	}

	SendInitialFacts();
}

void UAtlantisAudioFactBridgeComponent::UnbindSources()
{
	// Reset the facts listeners may still be holding, then say the player is gone.
	// Otherwise an alarm, or the music's Tension, would stay stuck after death or unpossession.
	if (GetWorld() && bSourcesBound)
	{
		FAtlantisAudioBoolPayload Payload;
		Payload.bValue = false;
		UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Oxygen_Critical, FInstancedStruct::Make(Payload));
		UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Stealth_Hiding, FInstancedStruct::Make(Payload));
		UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Player_Underwater, FInstancedStruct::Make(Payload));
		UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Listener_Underwater, FInstancedStruct::Make(Payload));
		UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Player_Present, FInstancedStruct::Make(Payload));
	}

	if (OxygenComponent)
	{
		OxygenComponent->OnCurrentOxygenChanged.RemoveDynamic(this, &ThisClass::HandleCurrentOxygenChanged);
		OxygenComponent->OnCriticalOxygenChanged.RemoveDynamic(this, &ThisClass::HandleCriticalOxygenChanged);
		OxygenComponent->OnAllocatedOxygenChanged.RemoveDynamic(this, &ThisClass::HandleAllocatedOxygenChanged);
		OxygenComponent->OnOxygenCapacityChanged.RemoveDynamic(this, &ThisClass::HandleOxygenCapacityChanged);
		OxygenComponent = nullptr;
	}

	if (HidingComponent)
	{
		HidingComponent->OnHidingStateChanged.RemoveDynamic(this, &ThisClass::HandleHidingStateChanged);
		HidingComponent = nullptr;
	}

	if (MovementComponent)
	{
		MovementComponent->OnUnderwaterChanged.RemoveDynamic(this, &ThisClass::HandleUnderwaterChanged);
		MovementComponent = nullptr;
	}

	// Cancel any pending player state retry or underwater exit, so nothing fires after unbind.
	// The world can already be gone if this component is being cleaned up during shutdown.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}

	PlayerStateRetryCount = 0;

	// The timers were cleared above, so forget their handles too.
	UnderwaterExitTimer.Invalidate();
	ListenerCheckTimer.Invalidate();

	bSourcesBound = false;
	bUnderwaterSent = false;
	bListenerUnderwaterSent = false;
	ListenerChangeHeldTime = 0.f;
}


void UAtlantisAudioFactBridgeComponent::SendInitialFacts()
{
	if (OxygenComponent)
	{
		SendOxygenLevel();
		HandleCriticalOxygenChanged(OxygenComponent->IsCriticalOxygen());
	}

	if (HidingComponent)
	{
		HandleHidingStateChanged(HidingComponent->IsHiding());
	}

	if (MovementComponent)
	{
		// The starting values are sent straight away, without either delay.
		SendUnderwater(MovementComponent->IsInWater());
		SendListenerUnderwater(IsListenerUnderwater());
	}

}

void UAtlantisAudioFactBridgeComponent::SendOxygenLevel()
{
	if (!OxygenComponent)
	{
		return;
	}

	const float Capacity = OxygenComponent->GetOxygenCapacity();

	FAtlantisAudioFloatPayload Payload;
	Payload.Value = Capacity > 0.f ? FMath::Clamp(OxygenComponent->GetAvailableOxygen() / Capacity, 0.f, 1.f) : 0.f;

	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Oxygen_Level, FInstancedStruct::Make(Payload));

}

void UAtlantisAudioFactBridgeComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	UnbindSources();
	TryBindSources();
}

void UAtlantisAudioFactBridgeComponent::HandleCurrentOxygenChanged(float OldCurrentOxygen, float NewCurrentOxygen)
{
	SendOxygenLevel();
}

void UAtlantisAudioFactBridgeComponent::HandleCriticalOxygenChanged(bool bCriticalOxygen)
{
	FAtlantisAudioBoolPayload Payload;
	Payload.bValue = bCriticalOxygen;
	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Oxygen_Critical, FInstancedStruct::Make(Payload));
}

void UAtlantisAudioFactBridgeComponent::HandleAllocatedOxygenChanged(float OldAllocatedOxygen, float NewAllocatedOxygen)
{
	SendOxygenLevel();
}

void UAtlantisAudioFactBridgeComponent::HandleOxygenCapacityChanged(float OldOxygenCapacity, float NewOxygenCapacity)
{
	SendOxygenLevel();
}

void UAtlantisAudioFactBridgeComponent::HandleHidingStateChanged(bool bHiding)
{
	FAtlantisAudioBoolPayload Payload;
	Payload.bValue = bHiding;

	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Stealth_Hiding, FInstancedStruct::Make(Payload));
}

void UAtlantisAudioFactBridgeComponent::HandleUnderwaterChanged(bool bUnderwater)
{
	// Use bUnderwater, not MovementComponent->IsInWater(). The event fires before the movement
	// component updates its volume, so IsInWater() still gives the old answer here.
	if (UWorld* World = GetWorld())
	{
		if (bUnderwater)
		{
			// Back in before the delay ran out, so listeners never hear the player left.
			World->GetTimerManager().ClearTimer(UnderwaterExitTimer);
			if (!bUnderwaterSent)
			{
				SendUnderwater(true);
			}
		}
		else
		{
			// Listeners already think the player is on land.
			if (!bUnderwaterSent)
			{
				return;
			}

			// Calling SetTimer again on the same handle restarts the countdown.
			World->GetTimerManager().SetTimer(
				UnderwaterExitTimer,
				this,
				&ThisClass::HandleUnderwaterExitTimer,
				UnderwaterExitDelay,
				false
			);
		}
	}
}

void UAtlantisAudioFactBridgeComponent::SendUnderwater(bool bUnderwater)
{
	FAtlantisAudioBoolPayload Payload;
	Payload.bValue = bUnderwater;
	bUnderwaterSent = bUnderwater;
	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Player_Underwater, FInstancedStruct::Make(Payload));
}

void UAtlantisAudioFactBridgeComponent::HandleUnderwaterExitTimer()
{
	SendUnderwater(false);
}

bool UAtlantisAudioFactBridgeComponent::IsListenerUnderwater() const
{
	if (!MovementComponent)
	{
		return false;
	}

	// Out of the water, the ears can't be in it either.
	APhysicsVolume* Volume = MovementComponent->GetPhysicsVolume();

	if (!Volume || !Volume->bWaterVolume)
	{
		return false;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());

	if (!Pawn)
	{
		return false;
	}

	APlayerController* Controller = Pawn->GetController<APlayerController>();

	if (!Controller)
	{
		return false;
	}

	// Only the location is used, but the function fills in all three.
	FVector ListenerLocation;
	FVector ListenerFront;
	FVector ListenerRight;

	Controller->GetAudioListenerPosition(ListenerLocation, ListenerFront, ListenerRight);

	return Volume->EncompassesPoint(ListenerLocation);
}

void UAtlantisAudioFactBridgeComponent::HandleListenerCheckTimer()
{
	const bool bListenerUnderwater = IsListenerUnderwater();

	// Nothing is changing, or a bob crossed back before the delay ran out.
	if (bListenerUnderwater == bListenerUnderwaterSent)
	{
		ListenerChangeHeldTime = 0.f;
		return;
	}

	ListenerChangeHeldTime += ListenerCheckInterval;

	if (ListenerChangeHeldTime >= ListenerChangeDelay)
	{
		SendListenerUnderwater(bListenerUnderwater);
		ListenerChangeHeldTime = 0.f;
	}
}

void UAtlantisAudioFactBridgeComponent::SendListenerUnderwater(bool bUnderwater)
{
	FAtlantisAudioBoolPayload Payload;
	Payload.bValue = bUnderwater;
	bListenerUnderwaterSent = bUnderwater;
	UAtlantisAudioDataBPFL::SetAudioDataPayload(this, TAG_AudioFact_Listener_Underwater, FInstancedStruct::Make(Payload));
}