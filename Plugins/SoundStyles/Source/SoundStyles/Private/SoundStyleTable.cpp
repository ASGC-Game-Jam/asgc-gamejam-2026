// Copyright (c) 2026 clarasmith-sound
// SPDX-License-Identifier: MIT

#include "SoundStyleTable.h"
#include "SoundStyleTags.h"
#include "SoundStyleSubsystem.h"   // for LogSoundStyles

// The lookup lives on the table (not the subsystem) so the table editor can use it without a running game.

const FSoundStyleEvent* USoundStyleTable::FindInClassChain(FGameplayTag ClassTag, FGameplayTag EventTag, FGameplayTag& OutRowTag) const
{
	for (FGameplayTag Tag = ClassTag; Tag.IsValid() && Tag != SoundStyles_Class; Tag = Tag.RequestDirectParent())
	{
		const FSoundStyleClass* Row = Classes.Find(Tag);
		if (Row)
		{
			const FSoundStyleEvent* Event = Row->Events.Find(EventTag);
			if(Event)
			{
				OutRowTag = Tag;
				return Event;
			}
		}
	}
	return nullptr;
}

const FSoundStyleEvent* USoundStyleTable::FindInClasses(const FGameplayTagContainer& LevelClasses, FGameplayTag EventTag, FGameplayTag& OutRowTag) const
{
	const FSoundStyleEvent* BestEvent = nullptr;
	int32 BestDepth = 0;
	FGameplayTag BestRowTag;

	for (const FGameplayTag& ClassTag : LevelClasses)
	{
		if(!ClassTag.MatchesTag(SoundStyles_Class))
		{
			UE_LOG(LogSoundStyles, Warning, TEXT("Sound class '%s' is not under UISound.Class and was ignored."), *ClassTag.ToString());
			continue;
		}

		FGameplayTag FoundTag;
		const FSoundStyleEvent* FoundEvent = FindInClassChain(ClassTag, EventTag, FoundTag);
		if(FoundEvent)
		{
			// Depth = number of tags in its chain (UISound, UISound.Class, ...Menu, ...Menu.Back). Deeper = more specific.
			const int32 FoundDepth = FoundTag.GetGameplayTagParents().Num();
			if(FoundDepth > BestDepth)
			{
				BestEvent = FoundEvent;
				BestDepth = FoundDepth;
				BestRowTag = FoundTag;
			}
			else if (FoundDepth == BestDepth && FoundTag != BestRowTag && USoundStyleSubsystem::IsDebugEnabled())
			{
				// Once per row pair (in listed order): this runs on every lookup, e.g. every hover.
				bool bAlreadyWarned = false;
				WarnedAmbiguousRowPairs.Add(TPair<FGameplayTag, FGameplayTag>(BestRowTag, FoundTag), &bAlreadyWarned);
				if (!bAlreadyWarned)
				{
					UE_LOG(LogSoundStyles, Warning, TEXT("Ambiguous UI sound: rows '%s' and '%s' both set '%s' at the same depth. Using '%s' (listed first). Give the widget a more specific class to choose."), *BestRowTag.ToString(), *FoundTag.ToString(), *EventTag.ToString(), *BestRowTag.ToString());
				}
			}
		}
	}

	OutRowTag = BestRowTag;
	return BestEvent;
}

const FSoundStyleEvent* USoundStyleTable::Resolve(const TArray<FGameplayTagContainer>& Levels, FGameplayTag EventTag, FGameplayTag& OutRowTag) const
{
	FGameplayTag FoundTag;
	for (const FGameplayTagContainer& Level : Levels)
	{
		const FSoundStyleEvent* FoundEvent = FindInClasses(Level, EventTag, FoundTag);
		if (FoundEvent)
		{
			OutRowTag = FoundTag;
			return FoundEvent;
		}
	}

	// No level answered: fall back to the root row (game-wide default).
	const FSoundStyleClass* Row = Classes.Find(SoundStyles_Class);
	if(Row)
	{
		const FSoundStyleEvent* FoundEvent = Row->Events.Find(EventTag);
		if(FoundEvent)
		{
			OutRowTag = SoundStyles_Class;
			return FoundEvent;
		}
	}
	return nullptr;
}