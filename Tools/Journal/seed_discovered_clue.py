"""Minimal #435 placeholder helpers for the Unreal Python console.

Load once:
exec(open(unreal.Paths.project_dir() + 'Tools/Journal/seed_discovered_clue.py').read())
Then use seed_clues() or clear_clues(), including in later PIE sessions.
"""
import unreal


def _current_journal():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
    if not world:
        raise RuntimeError('Start Play In Editor before using the journal helpers.')
    library = unreal.get_default_object(unreal.load_class(None, '/Script/Engine.SubsystemBlueprintLibrary'))
    return library.call_method('GetGameInstanceSubsystem', (world, unreal.AtlantisJournalSubsystem.static_class()))


def seed_clues():
    """Add the visible and hidden placeholders; repeated calls do not duplicate them."""
    journal = _current_journal()
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
    unreal.log('Placeholder clues ready. Use seed_clues() to reseed or clear_clues() to clear the journal.')


def clear_clues():
    """Clear all investigation entries in the current PIE session."""
    _current_journal().clear_investigations()
    unreal.log('Investigation entries cleared.')


seed_clues()
