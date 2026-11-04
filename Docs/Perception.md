# The Perception System
> It is highly recommended that you understand the basics behind Unreal's State Tree before editing its behavior.
> At the very least, I would recommend understanding how State Tree Evaluators work.
> I've added basic descriptions to StateTreePerceptionEvaluator.h, so look there if you want to get a quick run-down.

Right now, our Perception system leverages Unreal's AI perception system, and from that system, only Sight is implemented.

Because this project assumes that we can have multiple player characters, every enemy keeps an internal list of what player
characters they've seen along with flags that keep track of whether or not they've seen that player. This list of player characters
exists in the AtlantisEnemyController.h as a small struct.

How this works is simple: Enemies can see the player as long as they're within range. Designers can tweak this range
by looking at the BP_AtlantisEnemyController and adjusting parameters in the AIPerception component.
As a player is seen by an enemy, the enemy ticks up an internal Perception Value, and when this value hits 1, the enemy is set to aggro to the player.
The rate at which a player is seen can be adjusted in the ST_Enemy State Tree file, by checking its Perception Evaluator.

> State Tree Evaluators can be checked and their parameters modified on the left side of a State Tree Editor.

There also exists a smaller radius for which enemies instantly set the player's perception value to 1, and this can be similarly adjusted in the Perception Evaluator.

If an enemy loses sight of a player, a hysteresis timer is set, whose initial duration is determined from the Perception Evaluator's parameters.
After this hysteresis duration is past, an enemy's perception value of a player begins to degrade by a constant rate also determined by a Perception Evaluator's parameters.

Why have this hysteresis value? It prevents a player who darts in and out of sight have their perception value constantly flucuate, so enemies 'remember' players they've seen more constantly.

Once any player's perception value degrades to 0, an enemy 'forgets' them, by deleting their entry from the enemy's perceived players list.