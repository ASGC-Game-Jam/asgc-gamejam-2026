// Copyright (c) 2026 ASGC


#include "Core/AtlantisHidingComponent.h"
#include "Core/AtlantisPlayerCharacter.h"
#include "Core/AtlantisPlayerController.h"
#include "SmartObjectSubsystem.h"      
#include "SmartObjectRequestTypes.h"   
#include "GameFramework/Character.h"   
#include "Core/AtlantisHidingSpotBehaviourDef.h"    

namespace
{
	const FName HidingRequester = TEXT("Hiding");
}

UAtlantisHidingComponent::UAtlantisHidingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	
}

bool UAtlantisHidingComponent::EnterHidingSpot(AActor* HidingSpot)
{
	if (!HidingSpot)
	{
		return false;
	}

	if (ClaimHandle.IsValid())
	{
		return false;
	}

	AActor* Player = GetOwner();
	USmartObjectSubsystem* Subsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
	if (!Player || !Subsystem) {
		return false;
	}

	FSmartObjectRequestFilter Filter;
	Filter.BehaviorDefinitionClasses.Add(UAtlantisHidingSpotBehaviourDef::StaticClass());
	TArray<AActor*> ActorList;
	ActorList.Add(HidingSpot);
	TArray <FSmartObjectRequestResult> Results;
	

	if (!Subsystem->FindSmartObjectsInList(Filter, ActorList, Results, FConstStructView::Make(FSmartObjectActorUserData(Player))) || Results.Num() == 0)
	{
		return false;
	}

	const FSmartObjectRequestResult Result = Results[0];

	const FSmartObjectClaimHandle NewClaim = Subsystem->MarkSlotAsClaimed(Result.SlotHandle, ESmartObjectClaimPriority::Normal);
	if (!NewClaim.IsValid())
	{
		return false;
	}

	const UAtlantisHidingSpotBehaviourDef* Behaviour = Subsystem->MarkSlotAsOccupied<UAtlantisHidingSpotBehaviourDef>(NewClaim);
	if (!Behaviour)
	{
		Subsystem->MarkSlotAsFree(NewClaim);
		return false;
	}

	FTransform SlotTransform;
	if (!Subsystem->GetSlotTransform(NewClaim, SlotTransform))
	{
		Subsystem->MarkSlotAsFree(NewClaim);
		return false;
	}
	
	if (!ApplyHiding(SlotTransform))
	{
		Subsystem->MarkSlotAsFree(NewClaim);
		return false;
	}

	ClaimHandle = NewClaim;
	OnHidingStateChanged.Broadcast(ClaimHandle.IsValid());
	return true;
}

bool UAtlantisHidingComponent::ExitHidingSpot()
{
	if (!ClaimHandle.IsValid())
	{
		return false;
	}
	
	if (!RestoreFromHiding())
	{
		return false;
	}

	if (USmartObjectSubsystem* Subsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
	{
		Subsystem->MarkSlotAsFree(ClaimHandle);
	}

	ClaimHandle = FSmartObjectClaimHandle::InvalidHandle;
	OnHidingStateChanged.Broadcast(ClaimHandle.IsValid());
	return true;
}

bool UAtlantisHidingComponent::ApplyHiding(const FTransform& SlotTransform)
{
	
	AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(GetOwner());
	if (!PlayerCharacter)
	{
		return false;
	}
	
	PreviousTransform = PlayerCharacter->GetActorTransform();
	FTransform TargetTransform = SlotTransform;
	TargetTransform.SetScale3D(FVector::OneVector);
	PlayerCharacter->MoveToTransform(TargetTransform);
	PlayerCharacter->SetActorHiddenInGame(true);
	
	AAtlantisPlayerController* PlayerController = PlayerCharacter->GetController<AAtlantisPlayerController>();
	
	if (!PlayerController)
	{
		return false;
	}

	for (UInputMappingContext* MappingContext: RestrictedMappingContexts)
	{
		PlayerController->RestrictControls(HidingRequester, MappingContext);
	}

	PlayerController->SetControlRotation(SlotTransform.GetRotation().Rotator());

	return true;
}

bool UAtlantisHidingComponent::RestoreFromHiding()
{

	AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(GetOwner());
	if (!PlayerCharacter)
	{
		return false;
	}

	AAtlantisPlayerController* PlayerController = PlayerCharacter->GetController<AAtlantisPlayerController>();

	if (!PlayerController)
	{
		return false;
	}

	for (UInputMappingContext* MappingContext : RestrictedMappingContexts)
	{
		PlayerController->UnrestrictControls(HidingRequester, MappingContext);
	}
	
	PreviousTransform.SetRotation(PlayerController->GetControlRotation().Quaternion());
	PlayerCharacter->MoveToTransform(PreviousTransform);
	PlayerCharacter->SetActorHiddenInGame(false);

	return true;
}
