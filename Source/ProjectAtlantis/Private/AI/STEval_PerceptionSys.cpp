// Copyright (c) 2026 ASGC

#include "AI/STEval_PerceptionSys.h"

#include "StateTreeExecutionContext.h"
#include "Core/AtlantisEnemyController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

void FSTEval_PerceptionSys::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	Super::Tick(Context, DeltaTime);
	
	auto& InstanceData = Context.GetInstanceData<FSTEval_PerceptionSysInstanceData>(*this);
	auto* EnemyController = InstanceData.AIController.Get();
	if (EnemyController == nullptr)
	{
		UE_LOG(LogStateTree, Error, TEXT("[%hs] Failed to retrieve the AI Controller! (Is it set in the element's components?)"), __FUNCTION__);
		return;
	}
	
	auto* PerceptionComponent = EnemyController->GetPerceptionComponent();
	if (PerceptionComponent == nullptr)
	{
		UE_LOG(LogStateTree, Error, TEXT("[%hs] Failed to retrieve the Perception Component! (Does the operating actor have one in its controller?)"), __FUNCTION__);
		return;
	}
	
	// TODO: This is just Sight for now. Hearing will be later.
	TArray<AActor*> OutSensedActors;
	PerceptionComponent->GetHostileActorsBySense(UAISense_Sight::StaticClass(), OutSensedActors);
	
	if (OutSensedActors.Num() == 0)
		return;
}
