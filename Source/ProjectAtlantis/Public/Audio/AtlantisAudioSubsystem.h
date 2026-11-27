// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Audio/PubSubSystem/ASGC_FactListenerInterface.h"


#include "AtlantisAudioSubsystem.generated.h"



DECLARE_LOG_CATEGORY_EXTERN(LogAtlantisAudioSubsystem, Log, All);



class UAudioComponent;
class UMetasoundGeneratorHandle;
class UAtlantisMusicAxisMapping;
struct FAtlantisMusicAxisRule;


/**
 * Plays each level's music and drives its axes, layers and room type from audio facts.
 * Lives on the game instance, so it tracks each world as it loads and unloads.
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

    void UpdateMusicAxes();
    void UpdateLayerSplits();

    void UpdateRoomType();
    
    float GetRuleContribution(const FAtlantisMusicAxisRule& Rule) const;

#if WITH_EDITOR
    void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& Event);
    FDelegateHandle ObjectPropertyChangedHandle;
#endif

    // Handles for the world delegates, removed in StopTrackingWorld.
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

    UPROPERTY(Transient)
    TObjectPtr<UAtlantisMusicAxisMapping> AxisMapping;

	bool bIsActive = true;
    float RunningSweepTime = 0.f;
    float SweepTimerInterval = 0.033f;

    TMap<FGameplayTag, int32> FactSubscriptions;

    // Latest value of each fact, stored by payload shape and looked up by fact tag.
    TMap<FGameplayTag, float> FloatFacts;
    TMap<FGameplayTag, bool> BoolFacts;
    TMap<FGameplayTag, TMap<FName, FGameplayTag>> KeyedStateFacts;

    friend struct FAtlantisAudioSubsystemTestAccess;

    float CurrentPlayerState = 0.f;
    float CurrentTension = 0.f;
    

    TMap<FGameplayTag, FGameplayTag> TagFacts;
};
