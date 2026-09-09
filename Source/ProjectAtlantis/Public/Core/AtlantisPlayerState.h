// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AtlantisPlayerState.generated.h"

/** Oxygen supply changed. Carries the previous value so listeners can compute a delta. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOxygenCapacityChanged, float, OldOxygenCapacity, float, NewOxygenCapacity);

/**
 * Any part of the ballast system changed. Allocation and state are grouped into a single
 * event because nothing consumes one without the other.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallastChanged, bool, bIsAllocated, int32, BallastState);

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
	bool IsBallastAllocated() const { return bBallastAllocation; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	int32 GetBallastState() const { return BallastState; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	int32 GetTraversalMode() const { return TraversalMode; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	const TArray<FString>& GetEquippedItems() const { return EquippedItems; }

	//~ Mutators. Server only; calls on a client are ignored.
	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetOxygenCapacity(float NewOxygenCapacity);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetBallastAllocated(bool bNewBallastAllocation);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetBallastState(int32 NewBallastState);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetTraversalMode(int32 NewTraversalMode);

private:
	//PlayerState Variables - Often includes things like health, ammo etc.
	//TODO: note that these are placeholder variables and data types they may be swapped out for the real value upon implementation
	UPROPERTY(ReplicatedUsing = OnRep_OxygenCapacity, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	float OxygenCapacity = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_BallastAllocation, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	bool bBallastAllocation = false;

	UPROPERTY(ReplicatedUsing = OnRep_BallastState, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	int32 BallastState = 0;

	UPROPERTY(ReplicatedUsing = OnRep_TraversalMode, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	int32 TraversalMode = 0;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	TArray<FString> EquippedItems;

	//RepNotifies - These allow the server to notify clients of changes to replicated variables. It can alos be used as a change event when non-multiplayer
	UFUNCTION()
	void OnRep_OxygenCapacity(float OldOxygenCapacity) const;

	UFUNCTION()
	void OnRep_BallastAllocation(bool bOldBallastAllocation);

	UFUNCTION()
	void OnRep_BallastState(int32 OldBallastState);

	UFUNCTION()
	void OnRep_TraversalMode(int32 OldTraversalMode) const;

	UFUNCTION()
	void OnRep_EquippedItems(const TArray<FString>& OldEquippedItems) const;

	/** Shared fan-out for the two ballast properties. */
	void BroadcastBallastChanged() const;
};
