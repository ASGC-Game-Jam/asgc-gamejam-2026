// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"


#include "AtlantisAudioSubsystem.generated.h"



DECLARE_LOG_CATEGORY_EXTERN(LogAtlantisAudioSubsystem, Log, All);



class UAudioComponent;
class UMetasoundGeneratorHandle;

/**
 *  Manages the Music Subsystem for Project Atlantis
 */
UCLASS()
class PROJECTATLANTIS_API UAtlantisAudioSubsystem : public UGameInstanceSubsystem, public IASGC_FactListenerInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

    virtual void OnGameplayFactReceived_Implementation(FGameplayTag Tag, const FInstancedStruct& Payload) override;


private:
    void HandlePostWorldInitialization(UWorld* World, const UWorld::InitializationValues IValues);
    void HandleWorldBeginPlay();
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    void StopTrackingWorld();

    void UpdateAxisTestSweep();

    void SubscribeToFacts();
    void UnsubscribeFromFacts();

    // Stored Delegate Handles
    FDelegateHandle PostWorldInitHandle;
    FDelegateHandle WorldBeginPlayHandle;
    FDelegateHandle WorldCleanupHandle;

    FTimerHandle SweepTimerHandle;

    UPROPERTY()
    TWeakObjectPtr<UWorld> TrackedWorld;
    
    UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> MusicAudioComponent;

    UPROPERTY()
    TObjectPtr<UMetasoundGeneratorHandle> GeneratorHandle;   

	bool bIsActive = true;
    float RunningSweepTime = 0.f;
    float SweepTimerInterval = 0.033f;

    TMap<FGameplayTag, int32> FactSubscriptions;
};
