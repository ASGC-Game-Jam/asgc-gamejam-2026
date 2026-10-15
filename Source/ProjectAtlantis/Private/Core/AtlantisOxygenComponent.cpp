// Copyright (c) 2026 ASGC

#include "Core/AtlantisOxygenComponent.h"

#include "Net/UnrealNetwork.h"


AtlantisOxygenComponent::AtlantisOxygenComponent()
{
	PrimaryComponentTick.bCanEverTick = false; //Does it tick?!
	
	SetIsReplicatedByDefault(true);
}

void AtlantisOxygenComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AtlantisOxygenComponent, OxygenCapacity);
	DOREPLIFETIME(AtlantisOxygenComponent, AllocatedOxygen);
	DOREPLIFETIME(AtlantisOxygenComponent, CurrentOxygen);
	DOREPLIFETIME(AtlantisOxygenComponent, bCriticalOxygen);
}

void AtlantisOxygenComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasOwnerAuthority())
	{
		UpdateCriticalOxygen();
	}
}

void AtlantisOxygenComponent::CopyStateTo(AtlantisOxygenComponent& Target) const
{
	Target.OxygenCapacity = OxygenCapacity;
	Target.AllocatedOxygen = AllocatedOxygen;
	Target.CurrentOxygen = CurrentOxygen;
	Target.bCriticalOxygen = bCriticalOxygen;
}

void AtlantisOxygenComponent::SetOxygenCapacity(const float NewOxygenCapacity)
{
	if (!HasOwnerAuthority() ||FMath::IsNearlyEqual(OxygenCapacity, NewOxygenCapacity))
	{
		return;
	}

	const float OldOxygenCapacity = OxygenCapacity;
	OxygenCapacity = NewOxygenCapacity;

	OnRep_OxygenCapacity(OldOxygenCapacity);
}

void AtlantisOxygenComponent::SetAllocatedOxygen(const float NewAllocatedOxygen)
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


void AtlantisOxygenComponent::SetCurrentOxygen(const float NewCurrentOxygen)
{
	// Current oxygen includes the ballast allocation; consumption can only use the unlocked portion and cannot exceed capacity.
	const float ClampedOxygen = FMath::Clamp(NewCurrentOxygen, AllocatedOxygen, OxygenCapacity);
	if (!HasOwnerAuthority() || FMath::IsNearlyEqual(CurrentOxygen, ClampedOxygen))
	{
		return;
	}

	const float OldCurrentOxygen = CurrentOxygen;
	CurrentOxygen = ClampedOxygen;
	OnRep_CurrentOxygen(OldCurrentOxygen);
	UpdateCriticalOxygen();
}

void AtlantisOxygenComponent::SetCriticalOxygen(bool bNewCriticalOxygen)
{
	if (!HasOwnerAuthority() || bCriticalOxygen == bNewCriticalOxygen)
	{
		return;
	}

	bCriticalOxygen = bNewCriticalOxygen;
	OnRep_CriticalOxygen();
}

void AtlantisOxygenComponent::OnRep_OxygenCapacity(const float OldOxygenCapacity) const
{
	OnOxygenCapacityChanged.Broadcast(OldOxygenCapacity, OxygenCapacity);
}

void AtlantisOxygenComponent::OnRep_AllocatedOxygen(const float OldAllocatedOxygen) const
{
	OnAllocatedOxygenChanged.Broadcast(OldAllocatedOxygen, AllocatedOxygen);
}

void AtlantisOxygenComponent::OnRep_CurrentOxygen(const float OldCurrentOxygen) const
{
	OnCurrentOxygenChanged.Broadcast(OldCurrentOxygen, CurrentOxygen);
}

void AtlantisOxygenComponent::OnRep_CriticalOxygen() const
{
	OnCriticalOxygenChanged.Broadcast(bCriticalOxygen);
}

void AtlantisOxygenComponent::UpdateCriticalOxygen()
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

bool AtlantisOxygenComponent::RequestOxygen(const float RequestedOxygen)
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

bool AtlantisOxygenComponent::RequestOxygenAllocation(const float RequestedOxygen)
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

bool AtlantisOxygenComponent::ReleaseOxygen(const float ReleasedOxygen)
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

bool AtlantisOxygenComponent::HasOwnerAuthority() const
{
	return GetOwnerRole() == ROLE_Authority;
}