"""Minimal #435 placeholder helpers for the Unreal Python console.

Load once:
exec(open(unreal.Paths.project_dir() + 'Tools/Journal/seed_discovered_clue.py').read())
Then use seed_clues(), seed_grouped_clues(), revise_clue(), seed_puzzles(), or clear_clues().
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


def revise_clue(entry_id='Story435_Placeholder'):
    """Replace a seeded clue's current meaning without changing its identity or group."""
    journal = _current_journal()
    entry = next((item for item in journal.get_investigation_entries()
                  if item.entry_id == unreal.Name(entry_id)), None)
    if entry is None:
        raise RuntimeError('Seed the placeholder clue before revising it: ' + entry_id)
    entry.title = '[Placeholder] Revised clue'
    entry.descriptions = ['Revised placeholder interpretation; the previous text is replaced.']
    if not journal.set_investigation_entry(entry):
        raise RuntimeError('Revised clue was rejected: ' + entry_id)
    unreal.log('Clue revised in place. Reopen the journal or inspect it live to review the replacement.')


def clear_clues():
    """Clear all investigation entries in the current PIE session."""
    _current_journal().clear_investigations()
    unreal.log('Investigation entries cleared.')


seed_clues()


def seed_puzzles():
    """Seed #438 learned and undiscovered observations, without puzzle rules or answers."""
    journal = _current_journal()
    for identity, title, status in [
        ('Story438_Learned', '[Placeholder] Learned puzzle observation', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story438_Hidden', '[Placeholder] Hidden puzzle observation', unreal.JournalInvestigationStatus.UNDISCOVERED),
    ]:
        entry = unreal.JournalInvestigationEntry()
        entry.entry_id = identity
        entry.kind = unreal.JournalEntryKind.PUZZLE_OBSERVATION
        entry.title = title
        entry.descriptions = ['Placeholder observation only; no solution is supplied.']
        entry.status = status
        if not journal.set_investigation_entry(entry):
            raise RuntimeError('Puzzle placeholder was rejected: ' + identity)
    unreal.log('Puzzle placeholders ready in the existing Puzzle journal section.')


def seed_grouped_puzzles():
    """Seed #439 observations in two districts and three distinct puzzle groups."""
    journal = _current_journal()
    for identity, district, puzzle, status in [
        ('Story439_A1', 'A', 'One', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story439_A2', 'A', 'One', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story439_A3', 'A', 'Two', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story439_B1', 'B', 'One', unreal.JournalInvestigationStatus.INPROGRESS),
        ('Story439_Hidden', 'Hidden', 'Hidden', unreal.JournalInvestigationStatus.UNDISCOVERED),
    ]:
        entry = unreal.JournalInvestigationEntry()
        entry.entry_id = identity
        entry.kind = unreal.JournalEntryKind.PUZZLE_OBSERVATION
        entry.title = '[Placeholder] Puzzle observation ' + identity[-2:]
        entry.descriptions = ['A learned observation; no answer or solving logic.']
        entry.status = status
        entry.district_id = 'Story436_District_' + district
        entry.district_title = '[Placeholder] District ' + district
        entry.puzzle_id = 'Story439_Puzzle_' + puzzle
        entry.puzzle_title = '[Placeholder] Puzzle ' + puzzle
        if not journal.set_investigation_entry(entry):
            raise RuntimeError('Grouped puzzle placeholder was rejected: ' + identity)
    unreal.log('Grouped puzzle observations ready; hidden groups must remain absent.')
