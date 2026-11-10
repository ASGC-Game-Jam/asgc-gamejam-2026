// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AI/AtlantisNavNodeSource.h"
#include "Core/EAtlantisTraversalMode.h"
#include "AtlantisNavNode.generated.h"

class UBillboardComponent;

/**
 * Hand-placed waypoint for the swimming nav graph. Place it where a swimming enemy's capsule centre can be
 * (inside water, clear of geometry). Links to nearby nodes are built automatically.
 */
UCLASS(ClassGroup = (AI), meta = (DisplayName = "Atlantis Nav Node"))
class PROJECTATLANTIS_API AAtlantisNavNode : public AActor, public IAtlantisNavNodeSource
{
	GENERATED_BODY()

public:
	AAtlantisNavNode();

	//~ IAtlantisNavNodeSource
	virtual void GatherNavNodes(TArray<FAtlantisNavNodeDesc>& OutNodes) const override;

protected:
	/** How enemies may travel through this node. Only Swimming is supported for now. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Navigation", meta = (Bitmask, BitmaskEnum = "/Script/ProjectAtlantis.EAtlantisTraversalMode"))
	int32 TraversalModes;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};
