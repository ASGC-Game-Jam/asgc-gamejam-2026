// Copyright (c) 2026 ASGC

#include "Investigation/AtlantisInspectorComponent.h"
#include "Investigation/AtlantisInspectableComponent.h"
#include "Interaction/AtlantisInteractorComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PrimitiveComponent.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogAtlantisInspector, Log, All);

UAtlantisInspectorComponent::UAtlantisInspectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UAtlantisInspectorComponent::BeginInspect(UAtlantisInspectableComponent* Inspectable)
{
	if (!Inspectable || IsInspecting())
	{
		return false;
	}

	APlayerController* PlayerController = GetPlayerController();
	if (!PlayerController)
	{
		UE_LOG(LogAtlantisInspector, Warning, TEXT("%s on %s: not player controlled, can't inspect."),
			*GetName(), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!InspectMappingContext || !CloseAction)
	{
		UE_LOG(LogAtlantisInspector, Warning,
			TEXT("%s on %s: InspectMappingContext or CloseAction not set, refusing to inspect (the player could never close it)."),
			*GetName(), *GetNameSafe(GetOwner()));
		return false;
	}

	if (ScreenFillFraction <= 0.f)
	{
		UE_LOG(LogAtlantisInspector, Warning, TEXT("%s on %s: ScreenFillFraction not set, can't frame the object."),
			*GetName(), *GetNameSafe(GetOwner()));
		return false;
	}

	AActor* Object = Inspectable->GetOwner();
	const USceneComponent* ObjectRoot = Object ? Object->GetRootComponent() : nullptr;
	if (!ObjectRoot || ObjectRoot->Mobility != EComponentMobility::Movable)
	{
		UE_LOG(LogAtlantisInspector, Warning, TEXT("%s: root component must be Movable to be inspected."), *GetNameSafe(Object));
		return false;
	}

	InspectedObject = Inspectable;
	bInspecting = true;

	MoveObjectIntoView(Object, *Inspectable, *PlayerController);
	PushInspectInput(PlayerController);
	FreezeWorld();

	UE_LOG(LogAtlantisInspector, Log, TEXT("Inspecting %s (\"%s\")"),
		*GetNameSafe(Inspectable->GetOwner()), *Inspectable->DisplayName.ToString());
	OnInspectStarted.Broadcast(Inspectable);
	return true;
}

void UAtlantisInspectorComponent::EndInspect()
{
	if (!IsInspecting())
	{
		return;
	}

	// First, while everything is still in place, so listeners can clean up what they added
	OnInspectEnded.Broadcast(InspectedObject.Get());

	PopInspectInput();
	RestoreObject();
	UnfreezeWorld();

	UE_LOG(LogAtlantisInspector, Log, TEXT("Stopped inspecting %s"), *GetNameSafe(InspectedObject.IsValid() ? InspectedObject->GetOwner() : nullptr));
	InspectedObject.Reset();
	bInspecting = false;
}

void UAtlantisInspectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndInspect();
	Super::EndPlay(EndPlayReason);
}

APlayerController* UAtlantisInspectorComponent::GetPlayerController() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	return Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
}

void UAtlantisInspectorComponent::PushInspectInput(APlayerController* PlayerController)
{
	InspectInputComponent = NewObject<UEnhancedInputComponent>(PlayerController, UEnhancedInputComponent::StaticClass(), NAME_None, RF_Transient);
	InspectInputComponent->bBlockInput = true;
	InspectInputComponent->BindAction(CloseAction, ETriggerEvent::Started, this, &UAtlantisInspectorComponent::HandleCloseInput);
	if (RotateHoldAction && RotateAction)
	{
		InspectInputComponent->BindAction(RotateHoldAction, ETriggerEvent::Started, this, &UAtlantisInspectorComponent::HandleRotateHoldStarted);
		InspectInputComponent->BindAction(RotateHoldAction, ETriggerEvent::Completed, this, &UAtlantisInspectorComponent::HandleRotateHoldCompleted);
		InspectInputComponent->BindAction(RotateAction, ETriggerEvent::Triggered, this, &UAtlantisInspectorComponent::HandleRotate);
	}
	if (ZoomAction)
	{
		InspectInputComponent->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &UAtlantisInspectorComponent::HandleZoom);
	}
	PlayerController->PushInputComponent(InspectInputComponent);
	InputPlayerController = PlayerController;
	bRotateHeld = false;

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InspectMappingContext, InspectMappingPriority);
	}
}

void UAtlantisInspectorComponent::PopInspectInput()
{
	APlayerController* PlayerController = InputPlayerController.Get();
	if (PlayerController)
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->RemoveMappingContext(InspectMappingContext);
		}

		if (InspectInputComponent)
		{
			PlayerController->PopInputComponent(InspectInputComponent);
		}
	}

	InspectInputComponent = nullptr;
	InputPlayerController.Reset();
	bRotateHeld = false;
}

void UAtlantisInspectorComponent::HandleRotate(const FInputActionValue& Value)
{
	if (!bRotateHeld)
	{
		return;
	}

	// Turn around the camera's axes, not the object
	const FVector2D Delta = Value.Get<FVector2D>();
	const FQuat Yaw(ViewRotation.GetUpVector(), FMath::DegreesToRadians(-Delta.X * RotationSpeed));
	const FQuat Pitch(ViewRotation.GetRightVector(), FMath::DegreesToRadians(Delta.Y * RotationSpeed));
	ObjectRotation = (Yaw * Pitch * ObjectRotation).GetNormalized();
	ApplyObjectTransform();
}

void UAtlantisInspectorComponent::HandleZoom(const FInputActionValue& Value)
{
	ZoomScale = FMath::Clamp(ZoomScale - Value.Get<float>() * ZoomSpeed, MinZoomScale, MaxZoomScale);
	ApplyObjectTransform();
	OnInspectZoomChanged.Broadcast();
}

void UAtlantisInspectorComponent::MoveObjectIntoView(AActor* Object, const UAtlantisInspectableComponent& Inspectable, const APlayerController& PlayerController)
{
	MovedObject = Object;
	OriginalTransform = Object->GetActorTransform();
	bOriginalCollisionEnabled = Object->GetActorEnableCollision();

	// We disable physics bc we don't need them and collision would push the player around
	Object->SetActorEnableCollision(false);
	PausedPhysicsComponents.Reset();
	TArray<UPrimitiveComponent*> Primitives;
	Object->GetComponents<UPrimitiveComponent>(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (Primitive->IsSimulatingPhysics())
		{
			Primitive->SetSimulatePhysics(false);
			PausedPhysicsComponents.Add(Primitive);
		}
	}
	RefreshInteractorFocus();

	// Size: the object's bounding sphere, centered on its bounds (not its pivot, which artists often put at the bottom)
	FVector BoundsCenter;
	FVector BoundsExtent;
	Object->GetActorBounds(false, BoundsCenter, BoundsExtent);
	ObjectRadius = BoundsExtent.Size();

	// Camera
	const APlayerCameraManager* Camera = PlayerController.PlayerCameraManager;
	ViewLocation = Camera->GetCameraLocation();
	ViewRotation = Camera->GetCameraRotation().Quaternion();
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController.GetViewportSize(ViewportWidth, ViewportHeight);
	const float AspectRatio = ViewportHeight > 0 ? static_cast<float>(ViewportWidth) / ViewportHeight : 1.f;
	TanHalfHorizontalFOV = FMath::Tan(FMath::DegreesToRadians(Camera->GetFOVAngle() * 0.5f));
	TanHalfVerticalFOV = TanHalfHorizontalFOV / AspectRatio;

	// Distance at which the sphere's diameter covers ScreenFillFraction of the screen height
	FramedDistance = ObjectRadius /(ScreenFillFraction * TanHalfVerticalFOV) * Inspectable.DistanceMultiplier;
	FramedScreenOffset = ScreenOffset + Inspectable.ScreenOffsetNudge;

	// Facing the camera = turned half a circle from the camera's direction, then the object's own start rotation
	const FQuat FacingCamera = ViewRotation * FRotator(0.f, 180.f, 0.f).Quaternion();
	ObjectRotation = FacingCamera * Inspectable.StartRotation.Quaternion();
	ZoomScale = 1.f;

	ScaledLocalBoundsCenter = OriginalTransform.InverseTransformPosition(BoundsCenter) * OriginalTransform.GetScale3D();
	ApplyObjectTransform();
}

void UAtlantisInspectorComponent::ApplyObjectTransform()
{
	AActor* Object = MovedObject.Get();
	if (!Object)
	{
		return;
	}

	// The screen offset is recomputed at the zoomed distance, so zooming keeps the object at the same spot on screen
	const float Distance = FramedDistance * ZoomScale;
	const FVector TargetCenter = ViewLocation
		+ ViewRotation.GetForwardVector() * Distance
		+ ViewRotation.GetRightVector() * (FramedScreenOffset.X * Distance * TanHalfHorizontalFOV)
		+ ViewRotation.GetUpVector() * (FramedScreenOffset.Y * Distance * TanHalfVerticalFOV);
	
	const FVector Location = TargetCenter - ObjectRotation.RotateVector(ScaledLocalBoundsCenter);
	Object->SetActorLocationAndRotation(Location, ObjectRotation, false, nullptr, ETeleportType::TeleportPhysics);
}

void UAtlantisInspectorComponent::RestoreObject()
{
	if (AActor* Object = MovedObject.Get())
	{
		Object->SetActorTransform(OriginalTransform, false, nullptr, ETeleportType::TeleportPhysics);
		Object->SetActorEnableCollision(bOriginalCollisionEnabled);
		for (const TWeakObjectPtr<UPrimitiveComponent>& Primitive : PausedPhysicsComponents)
		{
			if (Primitive.IsValid())
			{
				Primitive->SetSimulatePhysics(true);
			}
		}
	}

	MovedObject.Reset();
	PausedPhysicsComponents.Reset();
}

void UAtlantisInspectorComponent::FreezeWorld()
{
	SavedTimeDilation = UGameplayStatics::GetGlobalTimeDilation(this);
	// The slowest the engine allows for this world
	UGameplayStatics::SetGlobalTimeDilation(this, GetWorld()->GetWorldSettings()->MinGlobalTimeDilation);
}

void UAtlantisInspectorComponent::UnfreezeWorld()
{
	UGameplayStatics::SetGlobalTimeDilation(this, SavedTimeDilation);
}

void UAtlantisInspectorComponent::RefreshInteractorFocus() const
{
	if (UAtlantisInteractorComponent* Interactor = GetOwner()->FindComponentByClass<UAtlantisInteractorComponent>())
	{
		Interactor->RefreshFocus();
	}
}
