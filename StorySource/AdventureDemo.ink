# Title: Inkpot Narative Adventure Demo

-> Main

/* This is the Ink file for the AdventureDemo level in the Inkpot Demo project.

It's designed to provide design patterns and examples of ways to implement common narrative adventure game functionality in Unreal Engine using Ink and Inkpot.

Checkout the AdventureDemoREADME for a walkthrough of how everything works.

The demo covers patterns for implementing:
    - Dialogue
    - Journal entries
    - Quests
*/

// FUNCTIONS - INK's BUILT-IN LIBRARY

=== function pop(ref _list) 
    ~ temp el = LIST_MIN(_list) 
    ~ _list -= el
    ~ return el 

=== function list_with_commas(list)
	{ list:
		{_list_with_commas(list, LIST_COUNT(list))}
	}

=== function _list_with_commas(list, n)
	{pop(list)}{ n > 1:{n == 2: and |, }{_list_with_commas(list, n-1)}}


// FUNCTIONS - Custom

// A !^ function that returns all items in list that are NOT in excluding.
=== function not_in(list, excluding)
    ~ temp disjoint = ()
    {excluding:
        ~ temp overlap = list ^ excluding
        ~ disjoint = LIST_INVERT(overlap) ^ list
    - else:
        ~ disjoint = list
    }
    
    ~ return disjoint

// STORY

VAR trust = 0

VAR clocktime = ""

LIST Items = cone, cube, sphere, watch
    VAR InventoryItems = ()
    VAR shapes = (cone, cube, sphere)

=== Main ===

{! Atlantis is sealing itself off district by district as the Bleaching spreads. The Oceanography Council has sent you down to find their missing contact, Sir Richard Black. # Linetime: 2.5} 

{
    - QuestMeetEmmy !? ToldEmmyJournal:
        -> Quests.MeetEmmy_quest
    - QuestGetTime !? ToldEmmyTime:
        -> Quests.GetTime_quest
    }
    
On to your next adventure.

-> DONE

=== NPC_Castella ===
{
    - QuestGetTime ? ToldEmmyTime:
        -> thanks
    - QuestGetTime ? (EmmyAsked, GotAnswer):
        -> answer
    - QuestGetTime ? EmmyAsked:
        -> ask
    - QuestMeetEmmy ? OpenedJournal:
        -> timequest
    - QuestMeetEmmy ? MetEmmy:
        -> intro.EmmyGivesJournal
    }
-> intro

= intro
Crates of waterlogged notebooks are stacked to the ceiling. A young jellyfolk woman in a bubble helmet and a healer's coat sits among them, hands clasped tight in her lap. She looks up at you with tired eyes. # Linetime: 2.5 italic: true

(wary)Hmmm… You're not one of the King's guards. And you're certainly not Richard.

So who let you in here?

/** {InventoryItems ? watch} [The time is {clocktime}]
    Thanks!
* {InventoryItems !? watch} [She wouldn't say.]
    To you either? I should steal her watch.*/
* [I'm a friend of Richard's. I came down to find him.]
    ~ trust += 5
    A friend. Hmmm. He has a lot of those, and lately none of them know where he is either.
    She sets down the notebook she was holding.
    The bulwarks closed every submarine route weeks ago. How did you even get down here?
    -> where_you_came_from
* [The door was open.]
    ~ trust -= 5
    (flat)It was not. I locked it myself.
    
    She sighs, and some of the fight goes out of her shoulders. # italic: true
    
    Fine. If you'd come to steal samples you'd have run by now. But the bulwarks closed every submarine route weeks ago. How did you get down here?
    -> where_you_came_from

Check out the EventGraph in the Level Blueprint to see how it's organized.

~ QuestMeetEmmy += MetEmmy

- (EmmyGivesJournal)

This is an example of some DIALOGUE. I'll add an entry on DIALOGUE to your JOURNAL. Plus an entry on JOURNAL ENTRIES. # Linetime: 2

~ JournalEntries += (journal, dialogue)

(Woah, meta).

{
    - QuestMeetEmmy ? OpenedJournal:
        -> timequest
    -else:
        Check those out by clicking on your Journal in the top right, then come back. (You may need to back away and approach me again to start a new conversaton). # Linetime: 2.5
    }

-> DONE
= where_you_came_from

    * [Through a cave. I startled something enormous, and the roof came down behind me.]
        ~ trust += 5
        (surprised and amused)Oh my goodness! Did you really do that?! You're remarkably impossible.
        For a moment she laughs like the young woman she is. Then the tiredness settles back over her face. # italic: true
        That was the Leviathan, you know. Half the city prays to it and the other half blames it for the Bleaching.
        -> hoping_to_find_richard
    * [Does it matter? I'm here now.]
        (sharp)It matters to me if you dragged the Bleaching in with you.
        She looks you over the way a doctor does: eyes, hands, the skin at your wrists. # italic: true
        No marks. Good. Sit down, then, and don't touch anything that glows.
        -> hoping_to_find_richard
-> DONE
= hoping_to_find_richard
Richard left all of this behind when he vanished. Notes, samples, half-finished drawings. I'm meant to sort it for the King's people, and I'm hoping it tells me where he went.
    * Let me help you go through it.
        ~ trust += 10
        -> tell_me_its_worth
    * What do his notes say about the Bleaching?
        -> tell_me_its_worth
-> DONE
= tell_me_its_worth
You work through the crates together. Near the bottom of one, a rolled schematic slides out from between two sample jars. # italic: true
Hmmm… this isn't medicine. It's a machine.
If his notes are right, this could push the bleached coral back. Even something the size of the Colossus. But look at the parts list. Half of it only exists in Districts 3 and 4, and the bulwarks are shut.
    She stares at the drawing for a long moment. # italic: true
(strained)This is all so much bigger than me… I feel like a bandage that is trying to stop an aneurysm. Please tell me it's worth something.
 * It's worth something. We'll find the parts, and we'll find Richard.
    A small, tired smile. # italic: true
    Richard used to say things like that. Usually right before doing something reckless.
    I can't carry all of this by myself. Will you trust me with his notes?
        ** I trust you.
            ~ trust += 5
            -> castella_talk_6
 * Honestly? I don't know yet.
    ~ trust += 5
    (relieved)Honest. Good.
    If someone's dying, I tell them. I'd rather you did the same for me than tell me pretty things.
        ** Then here's the truth: I'm not leaving until I find him.
            -> castella_talk_7
 * Then stop bandaging and focus on the machine.
    ~ trust -= 10
    (cold)Stop bandaging. Right. And let the people in front of me die while I tinker with drawings.
    Her clasped hands tighten until the knuckles pale. # italic: true
    I can't stop any of it. I'll save who I can, but what if I can't save even one?
        ** I'm sorry. That came out wrong.
            ~ trust += 5
            -> castella_talk_7
        ** Then the machine is the only real chance.
            -> castella_talk_7
-> DONE
= castella_talk_6
    I won't waste that.
    -> castella_talk_7
-> DONE
= castella_talk_7
There's an inventor out in the Wilds. Sabelle. If anyone can build Richard's machine, it's them.
Go on, then. Find out what the scientists in District 1 know. I'll keep digging here.
Conversation over. Castella's trust is now {trust}.
-> DONE
= timequest
~ QuestMeetEmmy += ToldEmmyJournal
{Great, now let's try a quest. |}

Can you ask Blue what time it is?

* (YouDoIt)[Can't you go ask Blue?]

    Well aren't you a fancypants narrative designer, expecting NPCs to advance their own plots instead of just standing around waiting for a hero. # Linetime: 2.5
    
    Unfortunately I don't have a character movement component--or even my own Blueprint-- so no I cannot ask Blue. # Linetime: 2.5
    
    Now, as much as I'd love to stand around discussing Jon Ingold's GDC talk on using LISTS as state machines for encounters in SORCERY--I have things to teach you about quest mechanics. # Linetime: 3
    
    -> timequest
    
* {QuestGetTime !? GotAnswer and InventoryItems !? watch} [Sure.]
    ~ JournalEntries += EmmyTime
    Thanks! <>
* {QuestGetTime ? GotAnswer and InventoryItems !? watch} [Blue doesn't like me very much.]
    Oh cool so the game told you to come talk to me and you wandered around the map antagonizing other NPCs instead. # Linetime: 2.5
    
    Nice work. # Linetime: 3
    
    I don't have a face, but I hope you picked up that the 3-second linetime on "Nice work" represented me staring at you judgmentally. # Linetime: 2.5
    
    (There's logic in WBP_Display_Adventure to pull in a "Linetime" tag from Ink and use it to set the timer for how long to keep the line up). # Linetime: 2.5
    
    Anyway. That's you finished with the tutorial. <>
    ~ QuestGetTime += ToldEmmyTime
* {InventoryItems ? watch} [It's {clocktime}.]
    Doing the quests out of order, I see. I respect it.
    ~ QuestGetTime += ToldEmmyTime

- I'm adding an entry on quests to your JOURNAL.
~ QuestGetTime += EmmyAsked
~ JournalEntries += quests
~ JournalEntries += EmmyTime

-> DONE

= ask
Hello again!
-> DONE

= answer
Did you ask Blue the time?

* {InventoryItems ? watch} [The time is {clocktime}]
    Thanks!
* {InventoryItems !? watch} [She wouldn't say.]
    To you either? I should steal her watch.

-
~ QuestGetTime += ToldEmmyTime

-> DONE

= thanks
Thanks for doing this tutorial.

-> DONE

=== NPC_Blue ===

{
    - QuestGetTime ? GotAnswer:
        -> hi_again
    - QuestFindShapes ? GotShapes:
        -> telltime
    - QuestGetTime ? AskedTime:
        -> hi_again
    - QuestGetTime ? EmmyAsked:
        -> asktime
}

-> blueIntro

= blueIntro

Hi there--could you help me out? I'm trying to find my {list_with_commas(shapes)}. # Linetime: 2.5

-> shapesFetch

= asktime

Hi, I'm Blue.

* [Emmy wants to know the time.]
* [Do you have the time?]
-
~ QuestGetTime += AskedTime

Blue: Sorry, I'm busy looking for my {list_with_commas(shapes)}. If you find them for me, I'll tell you the time. # Linetime: 2.5

-> shapesFetch

= shapesFetch
~ QuestFindShapes += BlueAsked

* [Sure, I can get them for you.]
    Thanks!
* [Can't you get them?]
    {NPC_Castella.timequest.YouDoIt: Yeah look I'm not a nerd like Emmy; I'm not going to wax philisophic about narrative design.} # Linetime: 2.5
    
    {QuestGetTime ? EmmyAsked: If you want to know the time, you'll need to get my shapes.|I can't get them myself because reasons--can you help me?} # Linetime: 2.5

- ~ JournalEntries += BlueShapes
-> DONE

= telltime
{QuestFindShapes ? BlueAsked: I see you've found them all|You found my missing shapes}! Thanks!

* [You're welcome.]
    Here, you can have my watch.
    ~InventoryItems += watch
* {QuestFindShapes ? BlueAsked}[They were literally right there.]
    If you're going to be rude, then {QuestGetTime ? EmmyAsked: it's time for you to get a watch|no quest reward for you}.
* {QuestFindShapes !? BlueAsked}[Finders Keepers.]
    Fine, no reward for you then.
   
-
~ QuestFindShapes += ToldBlue
~ QuestGetTime += GotAnswer
  
-> DONE

= hi_again
Hi again.

-> DONE

=== Quests ===
//Quest Trackers
/*  
    For a text adventure of this simplicity, we could rely on checking if knot/stitch labels evaluate true. (/Documentation/WritingWithInk.md#advanced-knotstitch-labels-are-actually-read-counts).

    But for interfacing with a game engine, it's simpler/cleaner to track quest state in a LIST so we can watch the quests variables for updates (for triggering UI etc) instead of having a soupy mess of stitch labels to monitor.
*/

LIST QuestMeetEmmy = MetEmmy, OpenedJournal, ToldEmmyJournal
LIST QuestGetTime = EmmyAsked, AskedTime, GotAnswer, ToldEmmyTime
LIST QuestFindShapes = BlueAsked, GotShapes, ToldBlue

-> DONE

= MeetEmmy_quest

{
    - QuestMeetEmmy !? MetEmmy:
        Go talk to Castella.
    - QuestMeetEmmy !? OpenedJournal:
      Check out the new entries in your journal, then head back to Emmy.
    - QuestMeetEmmy !? ToldEmmyJournal:
        You read your journal--head back and tell Emmy.
    - QuestMeetEmmy ? ToldEmmyJournal:
        You read your journal and told Emmy.
}

-> DONE

= GetTime_quest

{
    - QuestGetTime ? ToldEmmyTime: Quest Complete.
    - QuestGetTime ? GotAnswer: Go tell Emmy.
    - QuestGetTime ? AskedTime: -> FindShapes_quest
    - QuestGetTime ? EmmyAsked: go talk to Blue.
    - QuestGetTime !? EmmyAsked: Go talk to Emmy.
    }

-> DONE

= FindShapes_quest

{
    - QuestFindShapes ? ToldBlue:
        You found all of Blue's shapes and told her where they were.
    - InventoryItems ? (cone, cube, sphere):
        ~ QuestFindShapes += GotShapes
        You've found all the shapes. {QuestFindShapes ? BlueAsked:Head back to blue|I wonder if anyone's looking for these}.
    - QuestFindShapes ? BlueAsked:
        ~ temp foundshapes = InventoryItems ^ shapes
        Blue asked you to find her shapes before she'll give you the time. <>
        {foundshapes: You've found the {list_with_commas(foundshapes)}.} You still need the {list_with_commas(not_in(shapes, InventoryItems))}. # Linetime: 2.5
    - else:
        You've found a {list_with_commas(InventoryItems ^ shapes)}.
}

-> DONE

=== Journal ===
LIST JournalEntries = journal, dialogue, quests, EmmyTime, BlueShapes
VAR JournalBookmark = -> empty_entry

<- JournalBookmark

+ {JournalEntries has journal} [The Journal]
    ~ JournalBookmark = -> journal_entry
    -> Journal
+ {JournalEntries has quests} [Quests]
    ~ JournalBookmark = -> quests_entry
    -> Journal
+ {JournalEntries has dialogue} [Dialogue]
    ~ JournalBookmark = -> dialogue_entry
    -> Journal
+ {JournalEntries has EmmyTime} [Quest: Give Emmy the time]
    ~ JournalBookmark = -> EmmyTime_entry
    -> Journal
+ {JournalEntries has BlueShapes} [Quest: Find Blue's Shapes]
    ~ JournalBookmark = -> BlueShapes_entry
    -> Journal
    
- -> DONE

= empty_entry
This is the player journal.<br>

{
    - LIST_COUNT(JournalEntries) < 1:
        You don't have any entries yet--you'll get a notification in the top right when entries are added.
    - else:
        Select a Journal Entry from the menu to view.
}



-> DONE

= journal_entry
~ QuestMeetEmmy += OpenedJournal

This is the player character's Journal.<br>

In the Ink file, Journal entries each have their own stitch in the "Journal" knot.<br>

Full instructions for adding journal entries can be found in README-AdventureDemo.md.<br>

The quick version is that each entry is stored in a stitch within the Journal knot in the ink file. At the top of the knot, a sticky choice block handles the journal's table of contents, and lists all journal entries the player can currently see.<br>

The current entry the player is looking at is stored in the JournalBookmark variable in Ink. This ensures that when the player opens the journal, they'll be returned to the last viewed entry.<br>

-> DONE

= quests_entry

(Note: you can find this info in README-AdventureDemo.md).<br>

INK SETUP:<br>

Quests in this demo are making heavy use of LISTs as state machines to track the player's progress, as explained in the "Design Basics" section of README-AdventureDemo.md.<br>

We're again using flow-switching to know when to display quest text. Text for each quest is stored in its own stitch in the Quests knot. When `Switch Flow to Path` is called from Unreal to access the stitch, switching logic within the stitch will show the appropriate text depending on the player's progress.<br>

It's worth noting that quests don't need player-facing text of the sort you'd see in a quest log: you can track states of various storylines and use that information to impact the course of the narrative without popping up notices and instructions to the player. This demo uses pretty traditional style game quests in which the player runs errands to advance the story. But in a mystery game, for example: you might want to track the clues a player has seen without calling attention to the fact that they're important.<br>

UNREAL SETUP:<br>

At the moment, there's no Unreal-specific setup to do for quest text--it will be handled by the same Blueprint logic being used for dialogue.<br>

That's because there's no dedicated UI surface for Quest Text in this demo. Since it's not possible for the game to be in two flows at the same time, we can't display quest text and dialogue simultaneously--so they share a UI panel.<br>

(The journal also can't be displayed simultaneously, but since the Journal UI panel hides the main story panel, switching the flow to the journal and back is seamless for the player. It wouldn't make sense to interrupt dialogue to show quest updates).<br>

The upshot is that you'll need to think about when to trigger quest text to make sure it's not interrupting other narrative text.<br>

In the demo, we're triggering quest text when the player interacts with the shapes, and when the player leaves an NPC trigger volume and returns to the default flow. This makes sure it won't collide with dialogue.<br>

-> DONE

= EmmyTime_entry

Emmy asked you to ask Blue what time it is.

{
    - QuestGetTime ? ToldEmmyTime: You had to find Blue's shapes first, but {InventoryItems ? watch:she gave you her watch, so you were able to tell Emmy the time|she still refused to give you the time of day, and you had to tell Emmy}.
    - QuestGetTime ? GotAnswer:
        You told Blue where to find her shapes, {InventoryItems ? watch:and she gave you her watch|but she didn't like your tone, so she refused to tell you the time}.
        You should go tell Emmy.
    - QuestGetTime ? AskedTime: Blue won't give you the time until you  {QuestFindShapes < GotShapes:find her shapes and }tell her where to find {QuestFindShapes < GotShapes:them|her shapes}.
    - else: You should go talk to Blue.
    }

-> DONE

= BlueShapes_entry

Blue asked you to find her shapes before she'll give you the time.

{
    - QuestFindShapes ? ToldBlue:
        You found all three and told her where they were.
        
        {InventoryItems ? watch: Blue gave you her watch in exchange}.
    - InventoryItems ? (cone, cube, sphere):
        ~ QuestFindShapes += GotShapes
        You've found all the shapes. You should head back and tell Blue.
    - QuestFindShapes ? BlueAsked:
        ~ temp foundshapes = InventoryItems ^ shapes
        {foundshapes: You've found the {list_with_commas(foundshapes)}.} You still need the {list_with_commas(not_in(shapes, InventoryItems))}.
}

-> DONE

= dialogue_entry
~ QuestMeetEmmy += OpenedJournal
    
The Dialogue pattern in this demo is using trigger volumes to initiate dialogue for each NPC, and each NPC has their own knot in the AdventureDemo.ink: NPC_Emmy and NPC_Blue.<br>

The logic to update the UI with each line of dialogue and handle player choices lives in `WBP_Display_Adventure`.<br>

Check README-AdventureDemo.ink for an explanation of Inkpot concepts underpinning the dialogue implementation, including reading from Ink tags, switching flows, and switching flow to path.<br>

-> DONE