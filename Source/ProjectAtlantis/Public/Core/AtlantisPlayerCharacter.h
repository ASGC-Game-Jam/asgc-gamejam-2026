// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AtlantisPlayerCharacter.generated.h"

UCLASS()
class PROJECTATLANTIS_API AAtlantisPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AAtlantisPlayerCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

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
};
