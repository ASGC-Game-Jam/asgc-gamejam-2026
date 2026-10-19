// Copyright (c) 2026 ASGC

#include "Core/AtlantisAIAnimation.h"
#include "Algo/RandomShuffle.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	USkeletalMeshComponent* Mesh;
};

// Sets default values for this component's properties
UAtlantisAIAnimation::UAtlantisAIAnimation()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UAtlantisAIAnimation::BeginPlay()
{
	Super::BeginPlay();

	for (auto Pair : Animations)
	{
		PlayAnimationGroups.Add(Pair.Key, {});
		ResetAnimationGroup(Pair.Key);
	}

	AActor* Owner = GetOwner();
	if (Owner != nullptr)
	{
		Mesh = Owner->FindComponentByClass<USkeletalMeshComponent>();
	}
}

// Called every frame
void UAtlantisAIAnimation::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAtlantisAIAnimation::ResetAnimationGroup(const FString& Name)
{
	FAtlantisAIAnimationGroup* SourceAnimationGroup = Animations.Find(Name);
	if (SourceAnimationGroup != nullptr)
	{
		if (SourceAnimationGroup->bRandomizeGroup)
		{
			ShuffleRandomAnimations(Name);
		}
		else
		{
			SequenceAnimationGroup(Name);
		}
		UE_LOG(LogTemp, Warning, TEXT("Resetting animation group %s"), *Name);
	}
}

void UAtlantisAIAnimation::SequenceAnimationGroup(const FString& Name)
{
	FAtlantisAIAnimationGroup* SourceAnimationGroup = Animations.Find(Name);
	FAtlantisAIAnimationGroup* PlayAnimationGroup = PlayAnimationGroups.Find(Name);
	if (SourceAnimationGroup != nullptr && PlayAnimationGroup != nullptr)
	{
		TArray<UAnimationAsset*>& SourceAnimations = SourceAnimationGroup->AnimationList;
		TArray<UAnimationAsset*>& PlayAnimations = PlayAnimationGroup->AnimationList;

		PlayAnimations.Empty(SourceAnimations.Num());
		PlayAnimations.Append(SourceAnimations);
	}
}

void UAtlantisAIAnimation::ShuffleRandomAnimations(const FString& Name)
{
	FAtlantisAIAnimationGroup* SourceAnimationGroup = Animations.Find(Name);
	FAtlantisAIAnimationGroup* PlayAnimationGroup = PlayAnimationGroups.Find(Name);
	if (SourceAnimationGroup != nullptr && PlayAnimationGroup != nullptr)
	{
		TArray<UAnimationAsset*>& SourceAnimations = SourceAnimationGroup->AnimationList;
		TArray<UAnimationAsset*>& PlayAnimations = PlayAnimationGroup->AnimationList;

		PlayAnimations.Empty(SourceAnimations.Num());
		PlayAnimations.Append(SourceAnimations);
		Algo::RandomShuffle(PlayAnimations);
	}
}

UAnimationAsset* UAtlantisAIAnimation::GetNextAnimation(const FString& Name)
{
	FAtlantisAIAnimationGroup* PlayAnimationGroup = PlayAnimationGroups.Find(Name);
	UAnimationAsset* Result = nullptr;
	if (PlayAnimationGroup != nullptr)
	{
		TArray<UAnimationAsset*>& AnimationList = PlayAnimationGroup->AnimationList;
		if (AnimationList.Num() == 0)
		{
			ResetAnimationGroup(Name);
		}

		Result = AnimationList.Pop();
	}

	return Result;
}

void UAtlantisAIAnimation::PlayNextAnimation(const FString& Name, bool bIsLooping)
{
	UAnimationAsset* NextAnimation = GetNextAnimation(Name);
	if (NextAnimation != nullptr)
	{
		if (Mesh != nullptr)
		{
			Mesh->PlayAnimation(NextAnimation, bIsLooping);
			OnAnimationPlay.Broadcast(Name, NextAnimation->GetName());
		}
	}
}

float UAtlantisAIAnimation::GetNextAnimationDuration(const FString& Name)
{
	UAnimationAsset* NextAnimation = GetNextAnimation(Name);
	float Duration = 0.0f;
	if (NextAnimation != nullptr)
	{
		Duration = NextAnimation->GetPlayLength();
	}
	return Duration;
}