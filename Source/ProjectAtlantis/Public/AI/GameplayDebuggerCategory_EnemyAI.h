#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"
#include "Core/AtlantisEnemyController.h"

struct FPerceptionDebugData
{
	bool bIsPlayer = false;
	
	// If player is being debugged, grab all FPlayerPerceptionData that has this player.
	TSet<TObjectPtr<AAtlantisEnemyController>> EnemyPerceptionData;
	
	// If enemy is being debugged, grab their controller.
	TObjectPtr<AAtlantisEnemyController> DebugEnemyController;
};

class PROJECTATLANTIS_API FGameplayDebuggerCategory_EnemyAI : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_EnemyAI();
	
	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
	void PrintEnemyStatus(AAtlantisPlayerCharacter* PlayerCharacter, AController* Controller);

	virtual void CollectData(APlayerController* OwnerPlayerController, AActor* DebugActor) override;
	
	virtual void DrawData(APlayerController* OwnerPlayerController, FGameplayDebuggerCanvasContext& CanvasContext) override;
	
protected:
	FPerceptionDebugData Data;
	
	const float PointDrawRadius = 5.f;
	
	const float DrawDebugInfoRadius = 5000.f;
};
#endif //WITH_GAMEPLAY_DEBUGGER