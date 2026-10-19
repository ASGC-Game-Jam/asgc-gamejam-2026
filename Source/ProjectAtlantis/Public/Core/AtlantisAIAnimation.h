// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AtlantisAIAnimation.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAnimationPlay, FString, GroupName, FString, AnimationName);

USTRUCT(BlueprintType)
struct FAtlantisAIAnimationGroup
{
	GENERATED_BODY()

	// TODO: probably better to use UAnimMontage, for now to get this working use UAnimationAssets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Animations")
	TArray<UAnimationAsset*> AnimationList;
	
	bool bRandomizeGroup;
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTATLANTIS_API UAtlantisAIAnimation : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAtlantisAIAnimation();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Atlantis|Animation")
	void ResetAnimationGroup(const FString& Name);

	// Fills animation group Name in same order as source animations
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Animation")
	void SequenceAnimationGroup(const FString& Name);

	// Fills animation group Name in same order as source animations
	UFUNCTION(BlueprintCallable, Category = "Atlantis|Animation")
	void ShuffleRandomAnimations(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|Animation")
	UAnimationAsset* GetNextAnimation(const FString& Name);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|Animation")
	void PlayNextAnimation(const FString& Name, bool IsLooping);

	UFUNCTION(BlueprintCallable, Category = "Atlantis|Animation")
	float GetNextAnimationDuration(const FString& Name);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlantis|Animation")
	TMap<FString, FAtlantisAIAnimationGroup> Animations;

	UPROPERTY(BlueprintAssignable, Category = "Events|Atlantis|Animation")
	FOnAnimationPlay OnAnimationPlay;

private:
	TMap<FString, FAtlantisAIAnimationGroup> PlayAnimationGroups;
};
