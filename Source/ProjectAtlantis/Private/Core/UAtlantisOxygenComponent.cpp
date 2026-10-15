// Copyright (c) 2026 ASGC

#include "Core/UAtlantisOxygenComponent.h"

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


void UAtlantisOxygenComponent::SetCurrentOxygen(const float NewCurrentOxygen)
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

bool UAtlantisOxygenComponent::HasOwnerAuthority() const
{
	return GetOwnerRole() == ROLE_Authority;
}