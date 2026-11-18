# theme: dark
# author: Pruthvi Das
INCLUDE GlobalFunctions.ink
INCLUDE EavesdropOpportunity.ink

EXT. OUTSIDE THE BUBBLING THOUGHTS, KORALIS-3.

You look up at the giant fluorescent sign, a shining beacon deep within a pitch-black darkness, hung above a small opening of a cave-like structure. It's where the waypoint on your map is pointing to.

The sign itself is written in a strange language you don't understand; the words are assembled with a set of thick squiggles and strokes so tightly written together that it isn't easy to tell what a letter is or where the words begin and end.

You point your trustee LingoScanner toward the board, and wait a while before a cheery PING! gives you the word: "Bubbling Thoughts". This is the place indeed.

* [Go in.] -> inside_rh_office
 
=== inside_rh_office ===
~ Set_Trust(rh, 50)
~ Set_Trust(gb, 50)
INT. THE BUBBLING THOUGHTS OFFICE, KORALIS-3.

Entering the place, you see a walrus and a mermaid arguing about something...

* [Interrupt.] -> interrupt
* [Eavesdrop.] -> eavesdropping_conversation

= interrupt
{Dec_Trust(gb, 10)}
You pardon yourself. The two fumble and stop arguing immediately. While the walrus collects himself, the mermaid looks despondent. He didn't seem to appreciate the interruption...

<center>EXIT {Print_Speaker_Name(gb)}</center>

-> introduction

=== introduction ===

{Set_Speaker(rh, 3)} {Set_Dialogue("Ah, welcome. How may I help you? Come here to share some troubles, perhaps?")}

-> option_set_1

== option_set_1
* [I'm looking for a Sir Richard Black. He's missing.] -> ask_about_richard
* [Who are you?] ->  ask_about_rh

= ask_about_rh
{Set_Speaker(rh, 4)} {Set_Dialogue("Me? Why! I'm " + Print_Char_Name(rh) + ", a therapist! I don't often get clients, but surely someone must've referred me to you... no?")}

Well? Are you going to tell him?

* [(lie) "Umm... yeah, yeah! {Print_Char_Name(sb)} did!"]
    {Inc_Trust(rh, 10)}
    {Set_Speaker(rh, 1)} {Set_Dialogue("Oh! Splendid! Yes, a good soul, that " + Print_Char_Name(sb) + ", hm?")}
    ... {Print_Char_Name(sb)}'s not going to appreciate that when they find out you pinned this on them.
    {Set_Speaker(rh, 1)} {Set_Dialogue("Anyway... where were we?")}
    
* ["I'm on official Oceanographic Council business, actually."]
    {Set_Speaker(rh, 2)} {Set_Dialogue("Oh. Oh... well... I suppose that's understandable. It's all by the by...")}
    He didn't seem to take that well.
    {Set_Speaker(rh, 1)} {Set_Dialogue("Anyway... where were we?")}
    
* ["That doesn't concern you, old man."]
    {Dec_Trust(rh, 10)}
    {Set_Speaker(rh, 4)} {Set_Dialogue("Quite the mouth on you, my strange friend. Fine. State your business.")}

- -> option_set_1

=== ask_about_richard ===

The rotund walrus shifts in his seat uncomfortably, and clears his throat.

{Set_Speaker(rh, 3)} {Set_Dialogue("W-well, I suppose I may have heard of him... Sounds like a, erm, a teacher? Oh! Perhaps a respected figure! It's too bad he's missing, as you say...")}

* [He's my friend. Please, just help me out here.]
    {Inc_Trust(rh, 10)}
    {Set_Speaker(rh, 4)} {Set_Dialogue("Aw, alright... y-yes, I know of Sir Richard. He was my client, actually. We did a few sessions, him and I. He was supposed to turn up this week, but... *sigh* well, you know as much as I do now.")}

* [Hiding something? Guess I'll go tell the authorities...]
    ~ Dec_Trust(rh, 10)
    {Print_Trust(rh)}
    At the mere mention of the authorities, {Print_Char_Name(rh)} almost jumps off his seat as he screams 'WAIT!'.
    {Set_Speaker(rh, 6)} {Set_Dialogue("Must you truly resort to such barbaric hostility!?")}
    You say nothing. After a brief pause, he resumes talking.
    {Set_Speaker(rh, 6)} {Set_Dialogue("Yes! I know of the explorer, he's been seeing me for therapy sessions ever since he found accomodation in Koralis-1. But if you're hoping to get something more out of me, you've come to the wrong place. I haven't seen him for over a week.")}
    He takes a moment to collect himself. Then eyes you from top to bottom.
    {Set_Speaker(rh, 6)} {Set_Dialogue("Well!? Get out of my office!")}

- -> option_set_2

=== option_set_2 ===

* ["I'll need any records you may have of him."]
    {
    - t_rh >= 40:
        {Print_Trust(rh)}
        {Set_Speaker(rh, 6)} {Set_Dialogue("W-well... unfortunately, I can't quite *break* my client privilege, you see...")}
        And for a brief moment, {Print_Char_Name(rh)} falls into a deep thought...
        {Set_Speaker(rh, 6)} {Set_Dialogue("Ohh, very well. If it helps you find someone who's <b>lost</b>. Given the numerous clients' records I must sift through, it may take me some time, I'm sure you understand. In the meanwhile, could you help my friend, a fellow named " + Print_Char_Name(gb) + ". When you've finished with him, I promise you I will have your records ready!")}
        JOURNAL UPDATED: Go help {Print_Char_Name(gb)} with, well, whatever.
    - t_rh <= 30:
        {Print_Trust(rh)}
        {Set_Speaker(rh, 6)} {Set_Dialogue("Now WHY would I break client privilege for the likes of *you*?")}
        You beckon the seal given to you by the King at the livid walrus.
        {Set_Speaker(rh, 2)} {Set_Dialogue("... It'll take me time to find it, given the numerous clients' records I must sift through. Meanwhile... take yourself to " + Print_Char_Name(gb) + ". That fellow needs a hand with something, and I shall only help you find the records you need, IF you help <b>him.</b>")}
        He pauses.
        {Set_Speaker(rh, 6)} {Set_Dialogue("... And after this, I hope you do not pester me with your <b>bothersome</b> face again.")}
        JOURNAL UPDATED: Go help {Print_Char_Name(gb)} with, well, whatever.
    }
* ["(Side Quest Opportunity) What was the argument from earlier about?"]
    {
    - t_rh >= 40:
        {Print_Trust(rh)}
        {Set_Speaker(rh, 5)} {Set_Dialogue("It's just " + Print_Char_Name(gb) + " and his usual childishness. But I've told him -- there is no such thing as cleaning dishes around here! By Neptune, we're under water! But he <b>still wants a dishwasher!</b> *Sigh*... yet the boy seeks one.")}
        You might want to go talk to the mermaid about their dishwasher. Maybe you can help.
        
        JOURNAL UPDATED: Ask {Print_Char_Name(gb)} about dishwashers.
        
    - t_rh <= 30:
        {Print_Trust(rh)}
        The walrus guffaws at you.
        {Set_Speaker(rh, 6)} {Set_Dialogue("If you think this is our <b>bonding</b> moment where you want to try and help <b>me</b>, you can <b>forget</b> about it! Run along now, human.")}
    } -> option_set_2

- -> finish_quest

=== finish_quest ===

QUEST COMPLETE.

-> END
