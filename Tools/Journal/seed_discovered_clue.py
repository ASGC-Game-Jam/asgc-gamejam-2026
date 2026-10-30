"""Minimal #435 placeholder. Run in the Unreal Python console during PIE.

exec(open(unreal.Paths.project_dir() + 'Tools/Journal/seed_discovered_clue.py').read())
No production narrative or asset changes. Re-running replaces the same two entries.
"""
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
if not world:
    raise RuntimeError('Start Play In Editor before seeding the placeholder clue.')
library = unreal.get_default_object(unreal.load_class(None, '/Script/Engine.SubsystemBlueprintLibrary'))
journal = library.call_method('GetGameInstanceSubsystem', (world, unreal.AtlantisJournalSubsystem.static_class()))
for identity, title, status in [
    ('Story435_Placeholder', '[Placeholder] Discovered clue', unreal.JournalInvestigationStatus.INPROGRESS),
    ('Story435_Hidden', '[Placeholder] Undiscovered clue - must remain hidden', unreal.JournalInvestigationStatus.UNDISCOVERED),
]:
    entry = unreal.JournalInvestigationEntry()
    entry.entry_id = identity
    entry.title = title
    entry.descriptions = ['Placeholder clue content for journal verification.']
    entry.status = status
    if not journal.set_investigation_entry(entry):
        raise RuntimeError('Placeholder entry was rejected: ' + identity)
unreal.log('Journal #435 placeholder ready; open the Journal Investigation tab.')
