// Copyright (c) 2026 ASGC

#include "Core/AtlantisOxygenComponent.h"

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
		
		UpdateCriticalOxygen();
	}
}

void UAtlantisOxygenComponent::CopyStateTo(UAtlantisOxygenComponent& Target) const
{
	Target.OxygenCapacity = OxygenCapacity;
	Target.AllocatedOxygen = AllocatedOxygen;
	Target.CurrentOxygen = CurrentOxygen;
	Target.bCriticalOxygen = bCriticalOxygen;
}

void UAtlantisOxygenComponent::SetOxygenCapacity(const float NewOxygenCapacity)
{
	if (!HasOwnerAuthority() ||FMath::IsNearlyEqual(OxygenCapacity, NewOxygenCapacity))
	{
		return;
	}

	const float OldOxygenCapacity = OxygenCapacity;
	OxygenCapacity = NewOxygenCapacity;

	OnRep_OxygenCapacity(OldOxygenCapacity);
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
	UpdateCriticalOxygen();
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
		UpdateCriticalOxygen();
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

void UAtlantisOxygenComponent::UpdateCriticalOxygen()
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

bool UAtlantisOxygenComponent::RestoreOxygen(const float RestoreOxygenAmount)
{
	// This acts as the mirror opposite of RequestOxygen: where RequestOxygen subtracts, this adds
	bool status = false;
	if (!HasOwnerAuthority() || RestoreOxygenAmount < 0.0f) // TODO: do we want to be able to use negative numbers such as fake chargers or a chance of "leaking" oxygen instead when grabbing canisters etc?
	{
		status = false;

	}
	else
	{
		status = SetCurrentOxygen(CurrentOxygen + RestoreOxygenAmount);
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