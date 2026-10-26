// Copyright (c) 2026 ASGC

#include "Puzzle/AtlantisPuzzleActions.h"
#include "Puzzle/AtlantisPuzzleManager.h"
#include "Puzzle/AtlantisPuzzleObject.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/Script.h"

UAtlantisPuzzleAction_SetActorsVisible::UAtlantisPuzzleAction_SetActorsVisible()
{
	bRunInPreview = true;
}

TArray<AActor*> UAtlantisPuzzleAction_SetActorsVisible::GetActorsToChange() const
{
	TArray<AActor*> Actors;
	for (AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}
		Actors.AddUnique(Target);
		if (bIncludeAttachedActors)
		{
			// Unreal does not hide attached actors with their parent (unlike Unity's SetActive), so do it here.
			TArray<AActor*> Attached;
			Target->GetAttachedActors(Attached, /*bResetArray*/ true, /*bRecursivelyIncludeAttachedActors*/ true);
			for (AActor* Child : Attached)
			{
				Actors.AddUnique(Child);
			}
		}
	}
	return Actors;
}

void UAtlantisPuzzleAction_SetActorsVisible::Execute_Implementation(const FAtlantisPuzzleActionContext& Context)
{
	for (AActor* Target : GetActorsToChange())
	{
#if WITH_EDITOR
		if (Context.bIsPreview)
		{
			// Editor-only temporary hiding: what the designer sees changes, what is saved does not.
			Target->SetIsTemporarilyHiddenInEditor(!bVisible);
			continue;
		}
#endif
		if (Context.Manager)
		{
			Context.Manager->SetActorVisible(Target, bVisible);
		}
	}
}

void UAtlantisPuzzleAction_SetActorsVisible::ClearPreview_Implementation(const FAtlantisPuzzleActionContext& Context)
{
#if WITH_EDITOR
	for (AActor* Target : GetActorsToChange())
	{
		Target->SetIsTemporarilyHiddenInEditor(false);
	}
#endif
}

void UAtlantisPuzzleAction_SetActorsVisible::GetAffectedActors(TArray<AActor*>& OutActors) const
{
	for (AActor* Target : Targets)
	{
		if (Target)
		{
			OutActors.AddUnique(Target);
		}
	}
}

namespace
{
	TArray<FName> GetObjectStates(const AActor* Target)
	{
		if (!Target || !Target->Implements<UAtlantisPuzzleObject>())
		{
			return {};
		}
		// Blueprint objects answer through ProcessEvent, which editor worlds block without this guard.
		FEditorScriptExecutionGuard ScriptGuard;
		return IAtlantisPuzzleObject::Execute_GetPuzzleStates(Target);
	}
}

UAtlantisPuzzleAction_SetObjectState::UAtlantisPuzzleAction_SetObjectState()
{
	bRunInPreview = true;
}

void UAtlantisPuzzleAction_SetObjectState::Execute_Implementation(const FAtlantisPuzzleActionContext& Context)
{
	for (AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}
		if (!Target->Implements<UAtlantisPuzzleObject>())
		{
			UE_LOG(LogAtlantisPuzzle, Warning, TEXT("Set Object State: %s does not implement Atlantis Puzzle Object."),
			       *Target->GetActorNameOrLabel());
			continue;
		}
		IAtlantisPuzzleObject::Execute_OnPuzzleState(Target, State, Context.bIsPreview);
	}
}

void UAtlantisPuzzleAction_SetObjectState::ClearPreview_Implementation(const FAtlantisPuzzleActionContext& Context)
{
	// Back to each object's default state (the first it lists), as a preview.
	for (AActor* Target : Targets)
	{
		const TArray<FName> States = GetObjectStates(Target);
		if (States.Num() > 0)
		{
			IAtlantisPuzzleObject::Execute_OnPuzzleState(Target, States[0], /*bIsPreview*/ true);
		}
	}
}

TArray<FName> UAtlantisPuzzleAction_SetObjectState::GetStateOptions() const
{
	TArray<FName> Options;
	bool bFirst = true;
	for (const AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}
		const TArray<FName> States = GetObjectStates(Target);
		if (bFirst)
		{
			Options = States;
			bFirst = false;
		}
		else
		{
			// With several targets, only offer states they all understand.
			Options.RemoveAll([&States](const FName& Option) { return !States.Contains(Option); });
		}
	}
	return Options;
}

void UAtlantisPuzzleAction_SetObjectState::CollectProblems(TArray<FString>& OutProblems) const
{
	for (const AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}
		if (!Target->Implements<UAtlantisPuzzleObject>())
		{
			OutProblems.Add(FString::Printf(TEXT("Set Object State targets %s, which does not implement Atlantis Puzzle Object"),
			                                *Target->GetActorNameOrLabel()));
		}
		else if (State.IsNone())
		{
			OutProblems.Add(FString::Printf(TEXT("Set Object State on %s has no State picked"), *Target->GetActorNameOrLabel()));
		}
		else if (!GetObjectStates(Target).Contains(State))
		{
			OutProblems.Add(FString::Printf(TEXT("Set Object State sets %s to '%s', which is not one of its states"),
			                                *Target->GetActorNameOrLabel(), *State.ToString()));
		}
	}
}

void UAtlantisPuzzleAction_SetObjectState::GetAffectedActors(TArray<AActor*>& OutActors) const
{
	for (AActor* Target : Targets)
	{
		if (Target)
		{
			OutActors.AddUnique(Target);
		}
	}
}

void UAtlantisPuzzleAction_PlaySound::Execute_Implementation(const FAtlantisPuzzleActionContext& Context)
{
	if (!Sound)
	{
		return;
	}

	if (bPlay2D)
	{
		UGameplayStatics::PlaySound2D(this, Sound, VolumeMultiplier, PitchMultiplier);
		return;
	}

	const AActor* Location = GetSoundLocation(Context);
	if (!Location)
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("Play Sound has no location; set Play At or Play 2D."));
		return;
	}
	UGameplayStatics::PlaySoundAtLocation(this, Sound, Location->GetActorLocation(), VolumeMultiplier,
	                                      PitchMultiplier);
}

const AActor* UAtlantisPuzzleAction_PlaySound::GetSoundLocation(const FAtlantisPuzzleActionContext& Context) const
{
	if (PlayAt)
	{
		return PlayAt;
	}
	// Puzzle-level lists and tasks without an Interact With have no task actor; the manager sits at the puzzle.
	return Context.TaskActor ? Context.TaskActor.Get() : Context.Manager.Get();
}

void UAtlantisPuzzleAction_CompletePuzzleTask::Execute_Implementation(const FAtlantisPuzzleActionContext& Context)
{
	if (!Puzzle || TaskName.IsNone())
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("Complete Puzzle Task on %s: set both Puzzle and Task Name."),
		       Context.Manager ? *Context.Manager->GetName() : TEXT("(unknown puzzle)"));
		return;
	}
	const int32 TaskIndex = Puzzle->FindTaskByName(TaskName);
	if (TaskIndex == INDEX_NONE)
	{
		UE_LOG(LogAtlantisPuzzle, Warning, TEXT("Complete Puzzle Task: no task '%s' on %s."), *TaskName.ToString(),
		       *Puzzle->GetName());
		return;
	}
	// A locked target task stays locked; the larger puzzle decides when it can be completed.
	Puzzle->TryCompleteTask(TaskIndex, Context.Instigator);
}

UAtlantisPuzzleAction_SetComponentsActive::UAtlantisPuzzleAction_SetComponentsActive()
{
	ComponentClass = UFXSystemComponent::StaticClass();
}

void UAtlantisPuzzleAction_SetComponentsActive::Execute_Implementation(const FAtlantisPuzzleActionContext& Context)
{
	const TSubclassOf<UActorComponent> Class = ComponentClass ? ComponentClass : TSubclassOf<UActorComponent>(UActorComponent::StaticClass());
	for (const AActor* Target : Targets)
	{
		if (!Target)
		{
			continue;
		}
		TInlineComponentArray<UActorComponent*> Components;
		Target->GetComponents(Class, Components);
		for (UActorComponent* Component : Components)
		{
			Component->SetActive(bActive, /*bReset*/ bActive);
		}
	}
}
