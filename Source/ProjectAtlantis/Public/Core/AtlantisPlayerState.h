// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AtlantisPlayerState.generated.h"

UENUM(BlueprintType)
enum class EAtlantisBallastState : uint8
{
	/** Invalid sentinel used to detect a ballast state that was not explicitly initialized. */
	None = 0,
	/** Sink passively without ballast oxygen; allow horizontal swimming and lower surface walking. */
	Descend,
	/** Hold roughly the same depth; allow horizontal and vertical swimming, but no surface walking. */
	Wander,
	/** Rise passively using more ballast oxygen than Wander; allow horizontal swimming and upper surface walking. */
	Ascend,
};

/** Oxygen supply changed. Carries the previous value so listeners can compute a delta. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOxygenCapacityChanged, float, OldOxygenCapacity, float, NewOxygenCapacity);

/** Oxygen supply changed. Carries the previous value so listeners can compute a delta. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCurrentOxygenChanged, float, OldCurrentOxygen, float, NewCurrentOxygen);

/**
 * Any part of the ballast system changed. Allocation and state are grouped into a single
 * event because nothing consumes one without the other.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallastChanged, bool, bIsAllocated, EAtlantisBallastState, BallastState);

/** Traversal mode changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTraversalModeChanged, int32, OldTraversalMode, int32, NewTraversalMode);

/** Equipped item set changed. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquippedItemsChanged, const TArray<FString>&, NewEquippedItems);

/**
 * Per-player state that survives respawn. Every property here is server-authoritative:
 * clients read via the getters and react via the change delegates, and only the server
 * may call the setters.
 */
UCLASS()
class PROJECTATLANTIS_API AAtlantisPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AAtlantisPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Carries our custom state across seamless travel and PlayerState re-creation. */
	virtual void CopyProperties(APlayerState* PlayerState) override;

	//~ Change events. Bind from UI, audio, and VFX rather than polling.
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnOxygenCapacityChanged OnOxygenCapacityChanged;

	//~ Change events. Bind from UI, audio, and VFX rather than polling.
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnCurrentOxygenChanged OnCurrentOxygenChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnBallastChanged OnBallastChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnTraversalModeChanged OnTraversalModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	//~ Accessors.
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	float GetOxygenCapacity() const { return OxygenCapacity; }
	
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	float GetCurrentOxygen() const { return CurrentOxygen; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	bool IsBallastAllocated() const { return bBallastAllocation; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	EAtlantisBallastState GetBallastState() const { return BallastState; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	int32 GetTraversalMode() const { return TraversalMode; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	const TArray<FString>& GetEquippedItems() const { return EquippedItems; }

	//~ Mutators. Server only; calls on a client are ignored.
	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetOxygenCapacity(float NewOxygenCapacity);
	
	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetCurrentOxygen(float NewCurrentOxygen);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetBallastAllocated(bool bNewBallastAllocation);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetBallastState(EAtlantisBallastState NewBallastState);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetTraversalMode(int32 NewTraversalMode);

protected:
	//PlayerState Variables - Often includes things like health, ammo etc.
	//TODO: note that these are placeholder variables and data types they may be swapped out for the real value upon implementation

	/** Maximum oxygen the player can hold. */
	UPROPERTY(ReplicatedUsing = OnRep_OxygenCapacity, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState")
	float OxygenCapacity = 100.f;

	/** Oxygen currently available to the player. */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentOxygen, EditAnywhere, BlueprintReadOnly, Category = "Atlantis|PlayerState")
	float CurrentOxygen = 100.f;

	/** Whether oxygen has been allocated to ballast; allocation rules are not implemented yet. */
	UPROPERTY(ReplicatedUsing = OnRep_BallastAllocation, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState")
	bool bBallastAllocation = false;

	/** Current ballast mode; None marks a state that has not been initialized for gameplay. */
	UPROPERTY(ReplicatedUsing = OnRep_BallastState, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState")
	EAtlantisBallastState BallastState = EAtlantisBallastState::None;

	/** Current traversal mode, stored as a placeholder integer until traversal modes are defined. */
	UPROPERTY(ReplicatedUsing = OnRep_TraversalMode, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState")
	int32 TraversalMode = 0;

	/** Identifiers for the items currently equipped by the player. */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState")
	TArray<FString> EquippedItems;

	//RepNotifies - These allow the server to notify clients of changes to replicated variables. It can alos be used as a change event when non-multiplayer
	UFUNCTION()
	void OnRep_OxygenCapacity(float OldOxygenCapacity) const;
	
	UFUNCTION()
	void OnRep_CurrentOxygen(float OldCurrentOxygen) const;

	UFUNCTION()
	void OnRep_BallastAllocation(bool bOldBallastAllocation);

	UFUNCTION()
	void OnRep_BallastState(EAtlantisBallastState OldBallastState);

	UFUNCTION()
	void OnRep_TraversalMode(int32 OldTraversalMode) const;

	UFUNCTION()
	void OnRep_EquippedItems(const TArray<FString>& OldEquippedItems) const;

	/** Shared fan-out for the two ballast properties. */
	void BroadcastBallastChanged() const;
};
