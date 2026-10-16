// Copyright (c) 2026 ASGC
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisOxygenComponent.generated.h"


/** Oxygen supply changed. Carries the previous value so listeners can compute a delta. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOxygenCapacityChanged, float, OldOxygenCapacity, float,
                                             NewOxygenCapacity);

/** Amount of Oxygen allocated changed. Carries the previous value so listeners can compute a delta. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAllocatedOxygenChanged, float, OldAllocatedOxygen, float,
                                             NewAllocatedOxygen);

/** Oxygen supply changed. Carries the previous value so listeners can compute a delta. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentOxygenChanged, float, OldCurrentOxygen, float, NewCurrentOxygen);

/**
 * CriticalOxygen changed. True while oxygen level is critical.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCriticalOxygenChanged, bool, bCriticalOxygen);

UCLASS(ClassGroup = (Atlantis), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisOxygenComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisOxygenComponent();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void CopyStateTo(UAtlantisOxygenComponent& Target) const;

	//~ Change events. Bind from UI, audio, and VFX rather than polling.
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState|Oxygen")
	FOnOxygenCapacityChanged OnOxygenCapacityChanged;

	//~ Change events. Bind from UI, audio, and VFX rather than polling.
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState|Oxygen")
	FOnAllocatedOxygenChanged OnAllocatedOxygenChanged;

	//~ Change events. Bind from UI, audio, and VFX rather than polling.
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState|Oxygen")
	FOnCurrentOxygenChanged OnCurrentOxygenChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState|Oxygen")
	FOnCriticalOxygenChanged OnCriticalOxygenChanged;

	//~ Accessors.
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	float GetOxygenCapacity() const { return OxygenCapacity; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	float GetAllocatedOxygen() const { return AllocatedOxygen; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	float GetCurrentOxygen() const { return CurrentOxygen; }

	/** Oxygen available for consumption after excluding the ballast reservation. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	float GetAvailableOxygen() const { return FMath::Max(CurrentOxygen - AllocatedOxygen, 0.f); }

	/** Indicates whether oxygen level is critical. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	bool IsCriticalOxygen() const { return bCriticalOxygen; }

	/** Oxygen level is critical when AvailableOxygen is below this value. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	float GetCriticalOxygenThreshold() const { return CriticalOxygenThreshold; }

	/** Oxygen above the critical threshold required to leave Critical Oxygen. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState|Oxygen")
	float GetCriticalOxygenRecoveryMargin() const { return CriticalOxygenRecoveryMargin; }

	//~ Mutators. Server only; calls on a client are ignored.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Atlantis|PlayerState|Oxygen")
	void SetOxygenCapacity(float NewOxygenCapacity);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState|Oxygen")
	void SetAllocatedOxygen(float NewAllocatedOxygen);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState|Oxygen")
	void SetCurrentOxygen(float NewCurrentOxygen);
	
	/** Consumes the requested oxygen if enough is available. Returns whether the request was accepted. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Atlantis|PlayerState|Oxygen")
	bool RequestOxygen(float RequestedOxygen);

	/** Locks the requested oxygen if enough is available. Returns whether the request was accepted. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Atlantis|PlayerState|Oxygen")
	bool RequestOxygenAllocation(float RequestedOxygen);

	/** Releases the requested oxygen if enough is already locked. Returns whether the request was accepted. */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly,Category = "Atlantis|PlayerState|Oxygen")
	bool ReleaseOxygen(float RequestedOxygen);

private:


	/** Maximum oxygen the player can hold. */
	UPROPERTY(ReplicatedUsing = OnRep_OxygenCapacity, EditDefaultsOnly, BlueprintReadOnly,
		Category = "Atlantis|PlayerState|Oxygen", meta = (AllowPrivateAccess = "true"))
	float OxygenCapacity = 100.f;

	/** Oxygen reserved for ballast and unavailable for consumption. */
	UPROPERTY(ReplicatedUsing = OnRep_AllocatedOxygen, EditAnywhere, BlueprintReadOnly,
		Category = "Atlantis|PlayerState|Oxygen", meta = (AllowPrivateAccess = "true"))
	float AllocatedOxygen = 0.f;

	/** Total oxygen, including LockedOxygen. Consumption cannot reduce it below the reservation. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentOxygen, EditAnywhere, BlueprintReadOnly,
		Category = "Atlantis|PlayerState|Oxygen", meta = (AllowPrivateAccess = "true"))
	float CurrentOxygen = 100.f;

	/** Indicates whether oxygen level is critical. */
	UPROPERTY(ReplicatedUsing = OnRep_CriticalOxygen, BlueprintReadOnly, Category = "Atlantis|PlayerState|Oxygen",
		meta = (AllowPrivateAccess = "true"))
	bool bCriticalOxygen = false;

	/** Oxygen level is critical when AvailableOxygen is below this value. */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|PlayerState|Oxygen", meta = (ClampMin = "0.0"))
	float CriticalOxygenThreshold = 5.f;

	/** Oxygen above the critical threshold required to leave Critical Oxygen. */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|PlayerState|Oxygen", meta = (ClampMin = "0.0"))
	float CriticalOxygenRecoveryMargin = 1.f;

	//RepNotifies - These allow the server to notify clients of changes to replicated variables. It can alos be used as a change event when non-multiplayer
	UFUNCTION()
	void OnRep_OxygenCapacity(float OldOxygenCapacity) const;

	UFUNCTION()
	void OnRep_AllocatedOxygen(float OldAllocatedOxygen) const;

	UFUNCTION()
	void OnRep_CurrentOxygen(float OldCurrentOxygen) const;

	UFUNCTION()
	void OnRep_CriticalOxygen() const;

	/** Toggles the flag indicating whether oxygen is at critical level or not. */
	UFUNCTION()
	void SetCriticalOxygen(bool bNewCriticalOxygen);

	/** Recomputes Critical Oxygen from available Oxygen. */
	void UpdateCriticalOxygen();

	/** Returns true when the owning actor has network authority */
	bool HasOwnerAuthority() const;
};
