// Copyright (c) 2026 ASGC

#include "Core/AtlantisOxygenComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"


UAtlantisOxygenComponent::UAtlantisOxygenComponent()
{
	PrimaryComponentTick.bCanEverTick = false; //Does it tick?!

	SetIsReplicatedByDefault(true);
}

void UAtlantisOxygenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UAtlantisOxygenComponent, OxygenCapacity);
	DOREPLIFETIME(UAtlantisOxygenComponent, AllocatedOxygen);
	DOREPLIFETIME(UAtlantisOxygenComponent, CurrentOxygen);
	DOREPLIFETIME(UAtlantisOxygenComponent, bCriticalOxygen);
}

void UAtlantisOxygenComponent::BeginPlay()
{
	Super::BeginPlay();

	if (HasOwnerAuthority())
	{
		CurrentOxygen = OxygenCapacity;
		AllocatedOxygen = 0.f;

		UpdateOxygenThresholds();
	}
}

void UAtlantisOxygenComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(OxygenConsumptionTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UAtlantisOxygenComponent::CopyStateTo(UAtlantisOxygenComponent& Target) const
{
	Target.OxygenCapacity = OxygenCapacity;
	Target.AllocatedOxygen = AllocatedOxygen;
	Target.CurrentOxygen = CurrentOxygen;
	Target.bCriticalOxygen = bCriticalOxygen;
}

bool UAtlantisOxygenComponent::SetOxygenCapacity(const float NewOxygenCapacity)
{
	if (!HasOwnerAuthority() || FMath::IsNearlyEqual(OxygenCapacity, NewOxygenCapacity))
	{
		return false;
	}

	const float OldOxygenCapacity = OxygenCapacity;
	OxygenCapacity = NewOxygenCapacity;

	OnRep_OxygenCapacity(OldOxygenCapacity);
	
	return true;
}

bool UAtlantisOxygenComponent::RequestIncreaseOxygenCapacity(const float O2CapacityAmount)
{
	if (!HasOwnerAuthority() || O2CapacityAmount < 0.0f)
	{
		return false;
	}
	const float NewOxygenCapacity = OxygenCapacity + O2CapacityAmount;
	return SetOxygenCapacity(NewOxygenCapacity);
}

bool UAtlantisOxygenComponent::RequestDecreaseOxygenCapacity(const float O2CapacityAmount)
{
	if (!HasOwnerAuthority() || O2CapacityAmount < 0.0f)
	{
		return false;
	}
	const float NewOxygenCapacity = OxygenCapacity - O2CapacityAmount;
	bool bStatus = SetOxygenCapacity(NewOxygenCapacity); // capturing status here so that we can then check to reduce the current O2 before exiting.
	if (NewOxygenCapacity < CurrentOxygen)
	{
		const float O2Delta = CurrentOxygen - NewOxygenCapacity;
		RequestOxygen(O2Delta); // utilize this function to perform the decrease and notify the listeners
	}

	return bStatus;
}

void UAtlantisOxygenComponent::SetAllocatedOxygen(const float NewAllocatedOxygen)
{
	if (!HasOwnerAuthority() || FMath::IsNearlyEqual(AllocatedOxygen, NewAllocatedOxygen))
	{
		return;
	}

	const float OldAllocatedOxygen = AllocatedOxygen;
	AllocatedOxygen = NewAllocatedOxygen;

	OnRep_AllocatedOxygen(OldAllocatedOxygen);
	UpdateOxygenThresholds();
}

bool UAtlantisOxygenComponent::SetCurrentOxygen(const float NewCurrentOxygen)
{
	bool status = false;
	// Current oxygen includes the ballast allocation; consumption can only use the unlocked portion and cannot exceed capacity.
	const float ClampedOxygen = FMath::Clamp(NewCurrentOxygen, AllocatedOxygen, OxygenCapacity);
	if (!HasOwnerAuthority() || FMath::IsNearlyEqual(CurrentOxygen, ClampedOxygen))
	{
		status = false;
	}
	else
	{
		const float OldCurrentOxygen = CurrentOxygen;
		CurrentOxygen = ClampedOxygen;
		OnRep_CurrentOxygen(OldCurrentOxygen);
		UpdateOxygenThresholds();
		status = true;
	}
	return status;
}

void UAtlantisOxygenComponent::SetCriticalOxygen(bool bNewCriticalOxygen)
{
	if (!HasOwnerAuthority() || bCriticalOxygen == bNewCriticalOxygen)
	{
		return;
	}

	bCriticalOxygen = bNewCriticalOxygen;
	OnRep_CriticalOxygen();
}

void UAtlantisOxygenComponent::SetConsumingOxygen(bool bNewConsumingOxygen)
{
	UWorld* World = GetWorld();

	if (!HasOwnerAuthority() || !World || IsConsumingOxygen() == bNewConsumingOxygen)
	{
		return;
	}

	FTimerManager& TimerManager = World->GetTimerManager();

	if (bNewConsumingOxygen)
	{
		TimerManager.SetTimer(OxygenConsumptionTimerHandle, this, &UAtlantisOxygenComponent::ConsumeOxygen,
		                      OxygenConsumptionInterval, true);
	}
	else
	{
		TimerManager.ClearTimer(OxygenConsumptionTimerHandle);
	}
}

bool UAtlantisOxygenComponent::IsConsumingOxygen() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimerManager().IsTimerActive(OxygenConsumptionTimerHandle);
}

void UAtlantisOxygenComponent::ConsumeOxygen()
{
	const float AvailableOxygen = GetAvailableOxygen();

	if (AvailableOxygen <= UE_KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float ConsumedOxygen = OxygenConsumptionRate * OxygenConsumptionInterval;
	RequestOxygen(FMath::Min(AvailableOxygen, ConsumedOxygen));
}

void UAtlantisOxygenComponent::OnRep_OxygenCapacity(const float OldOxygenCapacity) const
{
	OnOxygenCapacityChanged.Broadcast(OldOxygenCapacity, OxygenCapacity);
}

void UAtlantisOxygenComponent::OnRep_AllocatedOxygen(const float OldAllocatedOxygen) const
{
	OnAllocatedOxygenChanged.Broadcast(OldAllocatedOxygen, AllocatedOxygen);
}

void UAtlantisOxygenComponent::OnRep_CurrentOxygen(const float OldCurrentOxygen) const
{
	OnCurrentOxygenChanged.Broadcast(OldCurrentOxygen, CurrentOxygen);
}

void UAtlantisOxygenComponent::OnRep_CriticalOxygen() const
{
	OnCriticalOxygenChanged.Broadcast(bCriticalOxygen);
}

void UAtlantisOxygenComponent::UpdateOxygenThresholds()
{
	const float AvailableOxygen = CurrentOxygen - AllocatedOxygen;
	if (!bCriticalOxygen && AvailableOxygen <= CriticalOxygenThreshold)
	{
		SetCriticalOxygen(true);
	}
	else if (bCriticalOxygen && AvailableOxygen > CriticalOxygenThreshold + CriticalOxygenRecoveryMargin)
	{
		SetCriticalOxygen(false);
	}
}

bool UAtlantisOxygenComponent::RequestOxygen(const float RequestedOxygen)
{
	if (!HasOwnerAuthority() || RequestedOxygen < 0.f)
	{
		return false;
	}
	const bool bRequestAccepted = GetAvailableOxygen() >= RequestedOxygen;

	if (bRequestAccepted)
	{
		SetCurrentOxygen(CurrentOxygen - RequestedOxygen);
	}

	return bRequestAccepted;
}

bool UAtlantisOxygenComponent::RestoreOxygen(const float RestoreOxygenAmount, const bool bRestoreFullFlag)
{
	// This acts as the mirror opposite of RequestOxygen: where RequestOxygen subtracts, this adds
	bool status = false;
	if (!HasOwnerAuthority() || RestoreOxygenAmount < 0.0f)
	{
		status = false;
	}
	else
	{
		// if flag then restore to full regardless of restore oxygen amount param
		const float NewO2 = bRestoreFullFlag ? OxygenCapacity : CurrentOxygen + RestoreOxygenAmount;
		status = SetCurrentOxygen(NewO2);
	}

	return status;
}

bool UAtlantisOxygenComponent::RequestOxygenAllocation(const float RequestedOxygen)
{
	if (!HasOwnerAuthority() || RequestedOxygen < 0.f)
	{
		return false;
	}
	const bool bRequestAccepted = GetAvailableOxygen() >= RequestedOxygen;

	if (bRequestAccepted)
	{
		SetAllocatedOxygen(AllocatedOxygen + RequestedOxygen);
	}

	return bRequestAccepted;
}

bool UAtlantisOxygenComponent::ReleaseOxygen(const float ReleasedOxygen)
{
	if (!HasOwnerAuthority() || ReleasedOxygen < 0.f)
	{
		return false;
	}
	const bool bRequestAccepted = AllocatedOxygen >= ReleasedOxygen;

	if (bRequestAccepted)
	{
		SetAllocatedOxygen(AllocatedOxygen - ReleasedOxygen);
	}

	return bRequestAccepted;
}

bool UAtlantisOxygenComponent::ReallocateOxygen(float NewRequestedOxygen)
{
	if (!HasOwnerAuthority() || NewRequestedOxygen < 0.f || NewRequestedOxygen > CurrentOxygen)
	{
		return false;
	}

	SetAllocatedOxygen(NewRequestedOxygen);
	return true;
}

bool UAtlantisOxygenComponent::HasOwnerAuthority() const
{
	return GetOwnerRole() == ROLE_Authority;
}
