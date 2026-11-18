# Tutorial tips

Two blueprints were added in order to make input tips display the correct key:

BP_AtlantisTextRenderer, a simple BP with a text renderer component to be used as a base tutorial tip.

BP_AtlantisInputTip, a child of BP_AtlantisTextRenderer containing the logic needed to update the displayed key accordingly to the input method and the key setting.

Limitations:

Gamepad keys use the engine's generic names (for example "Gamepad Face Button Bottom"). A future improvement could be displaying icons instead of button names.

Open questions:

Will players be able to change controls settings while in game or only in main menu? The logic doesn't currently account for changes on the fly.