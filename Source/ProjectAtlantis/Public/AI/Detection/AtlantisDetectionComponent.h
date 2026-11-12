// Copyright (c) 2026 ASGC

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AtlantisAITypes.h"
#include "AI/Detection/AtlantisPerceptionModel.h"
#include "AtlantisDetectionComponent.generated.h"

class AAIController;
class UAIPerceptionComponent;
class UCurveFloat;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAtlantisDetectionStatusChanged, EMonsterStatus, OldStatus, EMonsterStatus, NewStatus);

/**
 * Far Cry-style detection for one enemy. Add it to the enemy's AI controller (next to its AI Perception).
 *
 * Every player the sight sense reports fills a per-player meter (rate from SightDistanceCurve over 3D distance;
 * no instant detection). Hidden players (UAtlantisHidingComponent::IsHiding) count as unseen even in line of sight.
 * Status: Combat at a full meter > Alerted while seeing a player at >= SuspicionThreshold > Searching after losing
 * one (up to MaxSearchDuration) > Idle. Also keeps the search memory (origin + checked points) for the search tasks.
 *
 * Ticks by itself (independent of the State Tree). If the owner is an AAtlantisEnemyController it also pushes
 * status and meter to it (SetStatus / SetDetection), which drives the sight cone colour and the detection marker.
 * The "Atlantis Detection" State Tree evaluator exposes the results to the tree.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class PROJECTATLANTIS_API UAtlantisDetectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAtlantisDetectionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	EMonsterStatus GetStatus() const { return Status; }

	/** Meter (0-1) of the player this enemy is reacting to. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	float GetMeter() const { return SuspectMeter; }

	/** The player this enemy is reacting to (aggroed first, then seen & suspicious, then lost & searched for), or null. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	AActor* GetSuspect() const { return Suspect.Get(); }

	/** Where the suspect was last seen. False when there's no suspect. */
	UFUNCTION(BlueprintPure, Category = "Atlantis|AI|Detection")
	bool GetLastKnownLocation(FVector& OutLocation) const;

	/**
	 * Gives up on every player we can't see right now (meter, aggro and search forgotten). Status is recomputed
	 * immediately (usually Idle). Called by the Give Up Search task.
	 */
	UFUNCTION(BlueprintCallable, Category = "Atlantis|AI|Detection")
	void ForgetLostTargets();

	/** Starts a new search around Origin (clears checked points). Called by the Begin Search task. */
	void BeginSearch(const FVector& Origin) { SearchMemory.BeginSearch(Origin); }
	void MarkSearchPointVisited(const FVector& Point) { SearchMemory.MarkVisited(Point); }
	const FAtlantisSearchMemory& GetSearchMemory() const { return SearchMemory; }

	/** Read-only view of the per-player memory (debugging, tests). */
	const TMap<TWeakObjectPtr<AActor>, FAtlantisPlayerDetection>& GetPlayers() const { return Players; }

	FAtlantisDetectionTuning MakeTuning() const;

	/** Finds the component on an AI controller (or on the controller of a pawn). */
	static UAtlantisDetectionComponent* FindFor(const AActor* ControllerOrPawn);

	/** Fired whenever the status changes (e.g. Idle -> Alerted -> Combat -> Searching). */
	UPROPERTY(BlueprintAssignable, Category = "Atlantis|AI|Detection")
	FOnAtlantisDetectionStatusChanged OnStatusChanged;

	/** X = 3D distance to the player (cm), Y = meter gained per second while seen (1 = fully detected in one second). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection")
	TObjectPtr<UCurveFloat> SightDistanceCurve;

	/** Meter level (0-1) at which the enemy reacts: Alerted while it sees the player, Searching after losing them. Below it, the enemy carries on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection", meta = (ClampMin = 0, ClampMax = 1, UIMin = 0, UIMax = 1))
	float SuspicionThreshold = 0.3f;

	/** Longest time the enemy keeps searching after losing a suspicious player. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection", meta = (ClampMin = 0, UIMin = 0, ForceUnits = "s"))
	float MaxSearchDuration = 45.f;

	/** After losing sight of a player, how long the meter holds (and aggro lasts) before it starts to drain. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection", meta = (ClampMin = 0, UIMin = 0, ForceUnits = "s"))
	float GraceTime = 5.f;

	/** How fast the meter drains once the grace time is over, in meter (0-1) per second (0.1 = a full meter empties in 10 s). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Atlantis|AI|Detection", meta = (ClampMin = 0, UIMin = 0, UIMax = 1))
	float DecayPerSecond = 0.1f;

protected:
	virtual void BeginPlay() override;

private:
	void UpdateDetection(float DeltaTime);
	/** No pawn (unpossessed or dead): forget everything and drop back to Idle with an empty meter. */
	void ResetDetection();
	/** Picks the suspect, sets the status and pushes both to the owning enemy controller. */
	void RefreshStatus();
	/** Debug view, CVar Atlantis.AI.Detection.Draw 1: last known locations, search points, status line above the enemy. */
	void DrawDebug() const;

	/** True when the player can't be seen right now even in line of sight (e.g. inside a hiding spot). Seam S6. */
	static bool IsConcealed(const AActor* Target);

	UAIPerceptionComponent* GetPerception() const;

	TMap<TWeakObjectPtr<AActor>, FAtlantisPlayerDetection> Players;
	FAtlantisSearchMemory SearchMemory;

	EMonsterStatus Status = EMonsterStatus::Idle;
	TWeakObjectPtr<AActor> Suspect;
	float SuspectMeter = 0.f;
	FVector SuspectLastKnownLocation = FVector::ZeroVector;

	bool bHasLoggedMissingSetup = false;
};
