"""PIE regression: swim into each bubble using movement input, land, then walk out.

Only the setup position outside each pocket is teleported. Boundary crossings
use the actual character movement component and its collision sweeps.
"""
import unreal
import json
from pathlib import Path

_entry_world = unreal.EditorLevelLibrary.get_game_world()
assert _entry_world, 'Start PIE first'
_entry_pawn = unreal.GameplayStatics.get_player_pawn(_entry_world, 0)
_entry_move = _entry_pawn.get_component_by_class(unreal.CharacterMovementComponent)
_entry_state = _entry_pawn.get_editor_property('player_state')
_entry_oxygen = _entry_state.get_oxygen_component()
_entry_look = next(a for a in unreal.GameplayStatics.get_all_actors_of_class(_entry_world,unreal.PostProcessVolume)
                   if a.get_actor_label()=='POC_UnderwaterLook')
_entry_state.set_ballast_state(unreal.AtlantisBallastState.WANDER)
_entry_start = _entry_pawn.get_actor_transform()
_entry_pockets = [(22000,0,3500),(26500,1500,4300)]
_entry = {'index': -1, 'phase': 'approach', 'results': []}

def _entry_next():
    _entry['index'] += 1
    if _entry['index'] == len(_entry_pockets):
        unreal.unregister_slate_post_tick_callback(_entry['handle'])
        _entry_pawn.set_actor_transform(_entry_start, False, False)
        _entry_oxygen.restore_oxygen(0, True)
        Path(unreal.Paths.project_saved_dir(), 'OceanPOC_EntryValidation.json').write_text(json.dumps(_entry['results'],indent=2))
        unreal.log('OCEAN_ENTRY_VALIDATION_PASS')
        return
    x,y,floor = _entry_pockets[_entry['index']]
    _entry_pawn.set_actor_location(unreal.Vector(x-1200,y,floor+700), False, False)
    _entry_move.stop_movement_immediately()
    _entry_oxygen.restore_oxygen(0, True)
    _entry_oxygen.request_oxygen(40)
    _entry['phase'] = 'approach'
    _entry['time'] = unreal.GameplayStatics.get_time_seconds(_entry_world)

def _entry_tick(delta):
    if unreal.GameplayStatics.get_time_seconds(_entry_world) - _entry['time'] > 30:
        unreal.unregister_slate_post_tick_callback(_entry['handle'])
        unreal.log_error('OCEAN_ENTRY_TIMEOUT '+str(_entry['index'])+' '+_entry['phase']+' '+str(_entry_pawn.get_actor_location()))
        return
    x,y,floor = _entry_pockets[_entry['index']]
    position = _entry_pawn.get_actor_location()
    if _entry['phase'] == 'approach':
        if position.x < x-100:
            _entry_pawn.add_movement_input(unreal.Vector(1,0,0),1,False)
            return
        _entry_move.stop_movement_immediately()
        _entry['phase'] = 'land'
    if _entry['phase'] == 'land':
        if _entry_move.movement_mode != unreal.MovementMode.MOVE_WALKING:
            return
        assert not _entry_oxygen.is_consuming_oxygen()
        assert _entry_oxygen.get_current_oxygen() > 60
        assert not _entry_look.get_editor_property('enabled')
        _entry['results'].append({'pocket':_entry['index']+1,'entry':'passed','oxygen':_entry_oxygen.get_current_oxygen()})
        unreal.log('OCEAN_ENTRY_WALK_PASS '+str(_entry['index']+1))
        _entry['phase'] = 'exit'
    if _entry['phase'] == 'exit':
        if position.x < x+1200:
            _entry_pawn.add_movement_input(unreal.Vector(1,0,0),1,False)
            return
        assert _entry_move.movement_mode == unreal.MovementMode.MOVE_SWIMMING
        assert _entry_oxygen.is_consuming_oxygen()
        assert _entry_look.get_editor_property('enabled')
        _entry['results'][-1]['exit'] = 'passed'
        _entry_next()

_entry['handle'] = unreal.register_slate_post_tick_callback(_entry_tick)
_entry_next()
