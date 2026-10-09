// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/EAtlantisTraversalMode.h"
#include "AtlantisPlayerCharacter.generated.h"

UCLASS()
class PROJECTATLANTIS_API AAtlantisPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAtlantisPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void UnPossessed() override;

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
	
	/** Maps an engine movement mode to a traversal mode */
	static EAtlantisTraversalMode ToTraversalMode(EMovementMode MovementMode);
	
	/** Pushes the current traversal mode to the PlayerState. */
	void UpdateTraversalMode();
};
