// Copyright (c) 2026 ASGC

#include "AI/AtlantisNavNode.h"

#include "AI/AtlantisNavGraph.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"

AAtlantisNavNode::AAtlantisNavNode()
{
	PrimaryActorTick.bCanEverTick = false;
	SetHidden(true);
	SetActorEnableCollision(false);

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	TraversalModes = Atlantis::AI::TraversalModeToMask(EAtlantisTraversalMode::Swimming);

#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite != nullptr)
	{
		Sprite->SetupAttachment(RootComponent);
		Sprite->bIsScreenSizeScaled = true;
	}
#endif
}

void AAtlantisNavNode::GatherNavNodes(TArray<FAtlantisNavNodeDesc>& OutNodes) const
{
	OutNodes.Add({GetActorLocation(), static_cast<uint8>(TraversalModes)});
}
