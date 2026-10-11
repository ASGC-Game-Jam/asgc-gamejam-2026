// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "AtlantisCheatManager.generated.h"

/** A menu entry invokes the same command QA can use in the Unreal console. */
USTRUCT(BlueprintType)
struct FAtlantisDebugAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FString Command;
};

/** Development cheats; Unreal does not instantiate this manager in Shipping. */
UCLASS()
class PROJECTATLANTIS_API UAtlantisCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UAtlantisCheatManager();

	/** Returns the possessed Atlantis character to its initial spawn without reloading. */
	UFUNCTION(Exec, BlueprintCallable, Category = "Atlantis|Debug")
	void ResetPlayer();

	/** Override in Blueprint to append commands, including Cheat Manager Extension commands. */
	UFUNCTION(BlueprintNativeEvent, Category = "Atlantis|Debug")
	TArray<FAtlantisDebugAction> GetDebugActions() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Atlantis|Debug")
	TArray<FAtlantisDebugAction> DebugActions;
};
