#if WITH_GAMEPLAY_DEBUGGER
#include "AI/GameplayDebuggerCategory_EnemyAI.h"

#include "Core/AtlantisAICharacter.h"
#include "Core/AtlantisEnemyController.h"
#include "Core/AtlantisPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"


FGameplayDebuggerCategory_EnemyAI::FGameplayDebuggerCategory_EnemyAI()
{
	bShowOnlyWithDebugActor = true;
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_EnemyAI::MakeInstance()
{
	return MakeShared<FGameplayDebuggerCategory_EnemyAI>();
}

void FGameplayDebuggerCategory_EnemyAI::PrintEnemyStatus(AAtlantisPlayerCharacter* PlayerCharacter,
                                                         AController* Controller)
{
	if (PlayerCharacter == nullptr)
	{
		return;
	}
	FVector PlayerLocation = PlayerCharacter->GetActorLocation();
	AAtlantisEnemyController* EnemyController = Cast<AAtlantisEnemyController>(Controller);
	if (EnemyController == nullptr)
	{
		return;
	}
		
	AAtlantisAICharacter* AICharacter = Cast<AAtlantisAICharacter>(EnemyController->GetCharacter());
	if (AICharacter == nullptr)
	{
		return;
	}
		
	if (FMath::Square(DrawDebugInfoRadius) < FVector::DistSquared(PlayerLocation, AICharacter->GetActorLocation()))
	{
		return;
	}
		
	EMonsterStatus CurrentStatus = EnemyController->GetCurrentStatus();
		
	FColor PointColor;
	switch (CurrentStatus)
	{
	case EMonsterStatus::None:
		PointColor = FColor::White;
		break;
	case EMonsterStatus::Idle:
		PointColor = FColor::White;
		break;
	case EMonsterStatus::Alerted:
		PointColor = FColor::Yellow;
		break;
	case EMonsterStatus::Searching:
		PointColor = FColor::Orange;
		break;
	case EMonsterStatus::Combat:
		PointColor = FColor::Red;
		break;
	default: 
		PointColor = FColor::Purple;
		break;
	}
		
	TMap<TObjectPtr<AAtlantisPlayerCharacter>, FPlayerPerceptionData>& EnemyPerceivedActors = EnemyController->PerceivedActors;
	
	float PerceptionValueOnPlayerCharacter = 0.f;
	float HysteresisTime = 0.f;
	
	FPlayerPerceptionData* PerceptionData = EnemyPerceivedActors.Find(PlayerCharacter);
	
	if (PerceptionData != nullptr)
	{
		PerceptionValueOnPlayerCharacter = PerceptionData->PerceptionValue;
		HysteresisTime = PerceptionData->PerceptionHysteresisTime.Get(0.f);
	}

	AddShape(FGameplayDebuggerShape::MakePoint(
		AICharacter->GetActorLocation(),
		PointDrawRadius,
		PointColor,
		FString::Printf(TEXT("State: %s, Perception: %.2f, Hysteresis Time: %.2f"), *UEnum::GetValueAsString(CurrentStatus), PerceptionValueOnPlayerCharacter, HysteresisTime)
	));
}

void FGameplayDebuggerCategory_EnemyAI::CollectData(APlayerController* OwnerPlayerController, AActor* DebugActor)
{
	if (OwnerPlayerController == nullptr)
		return;

	auto* World = OwnerPlayerController->GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (DebugActor == nullptr)
	{
		return;
	}
	
	ResetReplicatedData();
	
	if (AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(DebugActor))
	{
		Data.bIsPlayer = true;
		
		// Yes, I know, Get All Actors of Class is terribly slow. If this was to be in production, then it'd be bad,
		// but since this is only with the gameplay debugger it shouldn't be in non-debug builds.
		TArray<AActor*> OutActors;
		UGameplayStatics::GetAllActorsOfClass(OwnerPlayerController, AAtlantisEnemyController::StaticClass(), OutActors);
		for (AActor* OutActor : OutActors)
		{
			AAtlantisEnemyController* EnemyController = Cast<AAtlantisEnemyController>(OutActor);
			if (EnemyController == nullptr)
			{
				continue;
			}
			
			if (EnemyController->PerceivedActors.Find(PlayerCharacter) == nullptr)
			{
				continue;
			}
			
			Data.EnemyPerceptionData.Add(EnemyController);
		}
	}
	else if (AAtlantisAICharacter* AICharacter = Cast<AAtlantisAICharacter>(DebugActor))
	{
		Data.bIsPlayer = false;
		Data.DebugEnemyController = AICharacter->GetController<AAtlantisEnemyController>();
	}
	
	AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(OwnerPlayerController->GetCharacter());
	if (PlayerCharacter == nullptr)
	{
		UE_LOG(LogAtlantisAI, Warning, TEXT("[%hs] Player Character could not be retrieved from the debugging controller."), __FUNCTION__);
		return;
	}
	
	// Yes, iterating through every single controller in the world is sketchy perf-wise.
	for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
	{
		PrintEnemyStatus(PlayerCharacter, It->Get());
	}
}

void FGameplayDebuggerCategory_EnemyAI::DrawData(APlayerController* OwnerPlayerController,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (Data.bIsPlayer)
	{
		CanvasContext.Printf(TEXT("Enemies that have perception info on this character (%s):"), *OwnerPlayerController->GetCharacter()->GetName());
		AAtlantisPlayerCharacter* PlayerCharacter = Cast<AAtlantisPlayerCharacter>(OwnerPlayerController->GetCharacter());
		if (PlayerCharacter == nullptr)
		{
			return;
		}
		
		for (AAtlantisEnemyController* EnemyController : Data.EnemyPerceptionData)
		{
			FPlayerPerceptionData* PerceptionStruct = EnemyController->PerceivedActors.Find(PlayerCharacter);
			if (PerceptionStruct == nullptr)
			{
				return;
			}
			
			FString EnemyName = EnemyController->GetCharacter()->GetName();
			FString StatusString = UEnum::GetValueAsString(EnemyController->GetCurrentStatus());
			
			float PerceptionValue = PerceptionStruct->PerceptionValue;
			float HysteresisTimeLeft = PerceptionStruct->PerceptionHysteresisTime.Get(0.f);
			
			CanvasContext.Printf(TEXT("[Enemy] %s - [Status] %s - [Perception] %.2f - [Hysteresis Time] %.2f"), *EnemyName, *StatusString, PerceptionValue, HysteresisTimeLeft);
		}
	}
	else
	{
		CanvasContext.Printf(TEXT("Current Status: %s"), *UEnum::GetValueAsString(Data.DebugEnemyController->GetCurrentStatus()));
		CanvasContext.Printf(TEXT("Players that this enemy (%s) has perception info on:"), *Data.DebugEnemyController->GetCharacter()->GetName());
		for (auto& PerceivedActor : Data.DebugEnemyController->PerceivedActors)
		{
			CanvasContext.Printf(TEXT("[Player] %s - [Perception] %.2f - [Hysteresis Time] %.2f"), *PerceivedActor.Key->GetName(), PerceivedActor.Value.PerceptionValue, PerceivedActor.Value.PerceptionHysteresisTime.Get(0.f));
		}
	}
}

#endif //WITH_GAMEPLAY_DEBUGGER