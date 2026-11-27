// Copyright (c) 2026 ASGC

#include "Audio/AtlantisAudioListenerComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisAudioListenerComponent, Log, All);

UAtlantisAudioListenerComponent::UAtlantisAudioListenerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Components tick before the camera updates by default, which would read last frame's camera.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UAtlantisAudioListenerComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerPawn = Cast<APawn>(GetOwner());
	if (!IsValid(OwnerPawn))
	{
		UE_LOG(LogAtlantisAudioListenerComponent, Error, TEXT("Audio listener component on [%s] must be on a pawn."), *GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}

	OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &ThisClass::HandleControllerChanged);
}

void UAtlantisAudioListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(OwnerPawn))
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &ThisClass::HandleControllerChanged);
		ClearOverride(OwnerPawn->GetController());
	}

	Super::EndPlay(EndPlayReason);
}

void UAtlantisAudioListenerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Only the local player has a listener.
	if (!IsValid(OwnerPawn) || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	// Asked for every tick, because possession can change the controller.
	APlayerController* PC = OwnerPawn->GetController<APlayerController>();
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return;
	}

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CharacterLocation = OwnerPawn->GetPawnViewLocation();
	const FVector ListenerLocation = FMath::Lerp(CameraLocation, CharacterLocation, ListenerBlend);

	// No component, so ListenerLocation is a world position.
	PC->SetAudioListenerAttenuationOverride(nullptr, ListenerLocation);
}

void UAtlantisAudioListenerComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	ClearOverride(OldController);
}

void UAtlantisAudioListenerComponent::ClearOverride(AController* Controller)
{
	// AI controllers have no listener.
	APlayerController* PC = Cast<APlayerController>(Controller);
	if (IsValid(PC))
	{
		PC->ClearAudioListenerAttenuationOverride();
	}
}
