![The arena](./MarkdownImages/TestArenaPresentation.png)

# Oxygen System Prototype

This is an initial prototype for the Oxygen System. This prototype intends only to show a rough idea about the system behaviour, not a principle for the programming explained in the TDD.

This prototype's code should be disregarded in the final implementation of the system.

## Affected Blueprints

 - All blueprints in Core/OxygenSystem;
 - BP_ThirdPersonCharacter;
 - BP_ThirdPersonGameMode.

## Project presentation

To show the player oxygen capacity, an oxygen level text meter has been placed over the player character head, showing how much oxygen the character has. The default maximum level is 100.

![Character with oxygen counter](./MarkdownImages/ScreenShot00000.png)

This level has two areas of interest in which the player character can walk into: the submerged area (A) and the oxygen refill area (B).

![The level areas](./MarkdownImages/TestArenaSections.png)

Each area is composed of a volume that detects when the player character enters it and sends a message to BP_ThirdPersonGameMode signaling that the Game Mode class should start depleting the character oxygen level.

Player character movement also cost oxygen: walking cost 0.1 oxygen unit each step and jumping cost 10 oxygen units. The code for these elements can be found in the BP_ThirdPersonCharacter Blueprint.

When the player enters the submerged volume, the oxygen counter begins to decrease, based in which movement the character is performing.

![Decreasing oxygen level](./MarkdownImages/ScreenShot00002.png)

When the playeer enters the oxygen refill area, the oxygen counters begins to increase.

![Increasing oxygen level](./MarkdownImages/ScreenShot00004.png)

These actions are performed in the Game Mode blueprint, BP_ThirdPersonGameMode. The player character only queries the Game Mode to fetch the oxygen level value.