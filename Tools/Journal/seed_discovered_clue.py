"""Minimal #435 placeholder helpers for the Unreal Python console.

Load once:
exec(open(unreal.Paths.project_dir() + 'Tools/Journal/seed_discovered_clue.py').read())
Then use seed_clues(), seed_grouped_clues(), or clear_clues(), including in later PIE sessions.
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


def seed_grouped_clues():
    """Seed #436 placeholders in two districts and three mystery groups."""
    journal = _current_journal()
    for identity, district, mystery, title, status in [
        ('Story436_A1', 'A', 'One', '[Placeholder] First clue', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story436_A2', 'A', 'One', '[Placeholder] Related clue', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story436_A3', 'A', 'Two', '[Placeholder] Separate mystery clue', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story436_B1', 'B', 'One', '[Placeholder] Other district clue', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story436_Hidden', 'Hidden', 'Hidden', 'Hidden placeholder', unreal.JournalInvestigationStatus.UNDISCOVERED),
    ]:
        entry = unreal.JournalInvestigationEntry()
        entry.entry_id = identity
        entry.title = title
        entry.descriptions = ['Placeholder content for district and mystery grouping verification.']
        entry.status = status
        entry.district_id = 'Story436_District_' + district
        entry.district_title = '[Placeholder] District ' + district
        entry.mystery_id = 'Story436_Mystery_' + mystery
        entry.mystery_title = '[Placeholder] Mystery ' + mystery
        if not journal.set_investigation_entry(entry):
            raise RuntimeError('Grouped placeholder was rejected: ' + identity)
    unreal.log('Grouped placeholders ready. The undiscovered district and mystery must remain hidden.')


def clear_clues():
    """Clear all investigation entries in the current PIE session."""
    _current_journal().clear_investigations()
    unreal.log('Investigation entries cleared.')


seed_clues()
