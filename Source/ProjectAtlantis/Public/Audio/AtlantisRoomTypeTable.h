// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "AtlantisRoomTypeTable.generated.h"

/** The one place room tags are matched to the IDs sent to the RoomType pin. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisRoomTypeTable : public UDataAsset
{
	GENERATED_BODY()

public:
	/** IDs must match the order of the room arrays in MSP_AtlantisMusicEnvironment. */
	UPROPERTY(EditAnywhere, Category = "Rooms", meta = (Categories = "Room"))
	TMap<FGameplayTag, int32> RoomTypeIds;
};
