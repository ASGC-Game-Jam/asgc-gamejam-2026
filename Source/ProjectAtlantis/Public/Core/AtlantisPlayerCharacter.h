// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/AtlantisTraversalTypes.h"
#include "AtlantisPlayerCharacter.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTraversalModeChanged, EAtlantisTraversalMode, NewMode, EAtlantisTraversalMode, OldMode);

UCLASS()
class PROJECTATLANTIS_API AAtlantisPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAtlantisPlayerCharacter(const FObjectInitializer& ObjectInitializer);
	
	/** It fires when the traversal mode changes */
	UPROPERTY(BlueprintAssignable, Category="Atlantis|Movement|Traversal")
	FOnTraversalModeChanged OnTraversalModeChanged;
	
	/** Getter for the current Traversal mode */ 
	UFUNCTION(BlueprintPure, Category = "Atlantis|Movement|Traversal")
	EAtlantisTraversalMode GetTraversalMode() const;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** Returns the character to the transform captured at BeginPlay. Authority only. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Movement")
	void MoveToStart();

	/** Moves the character to an arbitrary transform. Authority only. */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Movement")
	void MoveToTransform(const FTransform& Transform);

private:
	UPROPERTY(BlueprintReadOnly, Category = "Atlantis|Movement", meta = (AllowPrivateAccess = "true"))
	FTransform StartTransform;
	
	/** Current Traversal mode. Visible in the Details panel for debugging */
	UPROPERTY(VisibleInstanceOnly, Category = "Atlantis|Movement|Traversal")
	EAtlantisTraversalMode TraversalMode = EAtlantisTraversalMode::None;
	
	/** Maps and engiene movement mode to a traversal mode */
	static EAtlantisTraversalMode ToTraversalMode(EMovementMode MovementMode);
};
