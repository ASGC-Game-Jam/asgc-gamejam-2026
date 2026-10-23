// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Core/EAtlantisTraversalMode.h"

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


/**
 * Ballast mode changed. Includes the allocation status derived from locked oxygen.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallastChanged, bool, bIsAllocated, EAtlantisBallastState, BallastState);

/** Traversal mode changed(Terrestrial, Swimming, …). Carries the previous mode. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTraversalModeChanged, EAtlantisTraversalMode, OldTraversalMode, EAtlantisTraversalMode, NewTraversalMode);

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
	virtual void BeginPlay() override;
	
	AAtlantisPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnBallastChanged OnBallastChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnTraversalModeChanged OnTraversalModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Atlantis|PlayerState")
	FOnEquippedItemsChanged OnEquippedItemsChanged;

	//~ Accessors.
	UFUNCTION(BlueprintPure,Category="Atlantis|Oxygen")
	UAtlantisOxygenComponent* GetOxygenComponent() const { return OxygenComponent;}
	
	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	bool IsBallastAllocated() const;

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	EAtlantisBallastState GetBallastState() const { return BallastState; }

	/** Ballast permits player-controlled vertical swimming only in Wander. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Ballast")
	bool IsVerticalSwimmingAllowed() const { return BallastState == EAtlantisBallastState::Wander; }

	/** Ballast permits lower-surface attachment only in Descend; Surface Walking owns attachment. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Ballast")
	bool IsLowerSurfaceWalkingAllowed() const { return BallastState == EAtlantisBallastState::Descend; }

	/** Ballast permits upper-surface attachment only in Ascend; Surface Walking owns attachment. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|Ballast")
	bool IsUpperSurfaceWalkingAllowed() const { return BallastState == EAtlantisBallastState::Ascend; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	EAtlantisTraversalMode GetTraversalMode() const { return TraversalMode; }

	UFUNCTION(BlueprintPure, Category = "Atlantis|PlayerState")
	const TArray<FString>& GetEquippedItems() const { return EquippedItems; }

	/** Reserves oxygen for the requested mode only when total oxygen is strictly above its allocation. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetBallastState(EAtlantisBallastState NewBallastState);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|PlayerState")
	void SetTraversalMode(EAtlantisTraversalMode NewTraversalMode);
	
	/** Starts or stops oxygen consumption to match the IsUsingOxygen function */
	void RefreshOxygenConsumption() const;

protected:

	//PlayerState Variables - Often includes things like health, ammo etc.
	//TODO: note that these are placeholder variables and data types they may be swapped out for the real value upon implementation
	/** Carries our custom state across seamless travel and PlayerState re-creation. */
	virtual void CopyProperties(APlayerState* PlayerState) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Atlantis|Oxygen", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAtlantisOxygenComponent> OxygenComponent;
	
	/** Initial and current ballast mode; None is an invalid gameplay state. */
	UPROPERTY(ReplicatedUsing = OnRep_BallastState, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	EAtlantisBallastState BallastState = EAtlantisBallastState::Descend;

	/** Current traversal mode. Set by the player character from its movement mode; None when no pawn is possessed. */
	UPROPERTY(ReplicatedUsing = OnRep_TraversalMode, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	EAtlantisTraversalMode TraversalMode = EAtlantisTraversalMode::None;

	/** Identifiers for the items currently equipped by the player. */
	UPROPERTY(ReplicatedUsing = OnRep_EquippedItems, EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|PlayerState", meta = (AllowPrivateAccess = "true"))
	TArray<FString> EquippedItems;
	
	UFUNCTION()
	void OnRep_BallastState(EAtlantisBallastState OldBallastState) const;

	UFUNCTION()
	void OnRep_TraversalMode(EAtlantisTraversalMode OldTraversalMode) const;

	UFUNCTION()
	void OnRep_EquippedItems(const TArray<FString>& OldEquippedItems) const;
	
	UFUNCTION()
	void HandlePawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn);

	// Ballast reservations, not consumption rates; tune in Blueprint class defaults.
 
	/**	Required Oxygen Allocation for Descending Ballast State. */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|PlayerState", meta = (ClampMin = "0.0"))
	float DescendOxygenAllocation = 0.f;
	
	/**	Required Oxygen Allocation for Wander Ballast State. */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|PlayerState", meta = (ClampMin = "0.0"))
	float WanderOxygenAllocation = 1.f;

	/**	Required Oxygen Allocation for Ascending Ballast State. */
	UPROPERTY(EditDefaultsOnly, Category = "Atlantis|PlayerState", meta = (ClampMin = "0.0"))
	float AscendOxygenAllocation = 2.f;

	/** Returns the Oxygen allocation required by BallastState. */
	float GetRequiredBallastAllocation(EAtlantisBallastState BallastState) const;
	
	/** True while the player breathes underwater */
	bool IsUsingOxygen() const;
};
