"""Run during PIE. Validates the real pawn and native transitions, then returns to the start."""
import unreal
import json
from pathlib import Path

_ocean_world = unreal.EditorLevelLibrary.get_game_world()
assert _ocean_world, 'Start PIE first'
_ocean_pawn = unreal.GameplayStatics.get_player_pawn(_ocean_world, 0)
assert isinstance(_ocean_pawn, unreal.AtlantisPlayerCharacter)
_ocean_movement = _ocean_pawn.get_component_by_class(unreal.CharacterMovementComponent)
_ocean_state = _ocean_pawn.get_editor_property('player_state')
_ocean_oxygen = _ocean_state.get_oxygen_component()
_ocean_state.set_ballast_state(unreal.AtlantisBallastState.WANDER)
_ocean_start = _ocean_pawn.get_actor_transform()
_ocean_stages = [
    ('swimming', (20200,-1200,4400), unreal.MovementMode.MOVE_SWIMMING, True),
    ('air pocket 1', (22000,0,3620), unreal.MovementMode.MOVE_WALKING, False),
    ('left pocket 1', (24000,0,4800), unreal.MovementMode.MOVE_SWIMMING, True),
    ('air pocket 2', (26500,1500,4420), unreal.MovementMode.MOVE_WALKING, False),
    ('left pocket 2', (25000,0,4800), unreal.MovementMode.MOVE_SWIMMING, True),
]
_ocean_qa = {'index': -1, 'elapsed': 0.0, 'results': []}

def _ocean_next_stage():
    _ocean_qa['index'] += 1
    _ocean_qa['elapsed'] = 0.0
    if _ocean_qa['index'] >= len(_ocean_stages):
        unreal.unregister_slate_post_tick_callback(_ocean_qa['handle'])
        _ocean_pawn.set_actor_transform(_ocean_start, False, False)
        _ocean_oxygen.restore_oxygen(0, True)
        Path(unreal.Paths.project_saved_dir(), 'OceanPOC_Validation.json').write_text(json.dumps(_ocean_qa['results'], indent=2))
        unreal.log('OCEAN_POC_VALIDATION_PASS')
        return
    stage = _ocean_stages[_ocean_qa['index']]
    _ocean_pawn.set_actor_location(unreal.Vector(*stage[1]), False, False)
    _ocean_movement.stop_movement_immediately()
    _ocean_oxygen.restore_oxygen(0, True)
    _ocean_oxygen.request_oxygen(30)
    _ocean_qa['oxygen_before'] = _ocean_oxygen.get_current_oxygen()

def _ocean_tick(delta):
    _ocean_qa['elapsed'] += delta
    if _ocean_qa['elapsed'] < 2.0:
        return
    label, _, expected_mode, expected_drain = _ocean_stages[_ocean_qa['index']]
    current = _ocean_oxygen.get_current_oxygen()
    passed = (_ocean_movement.movement_mode == expected_mode
              and _ocean_oxygen.is_consuming_oxygen() == expected_drain
              and (current < _ocean_qa['oxygen_before'] if expected_drain else current > _ocean_qa['oxygen_before']))
    result = {'stage': label, 'passed': passed, 'mode': str(_ocean_movement.movement_mode),
              'consuming': _ocean_oxygen.is_consuming_oxygen(), 'oxygen': current}
    _ocean_qa['results'].append(result)
    unreal.log('OCEAN_POC_STAGE ' + json.dumps(result))
    if not passed:
        unreal.unregister_slate_post_tick_callback(_ocean_qa['handle'])
        unreal.log_error('OCEAN_POC_VALIDATION_FAILED ' + label)
        return
    _ocean_next_stage()

_ocean_qa['handle'] = unreal.register_slate_post_tick_callback(_ocean_tick)
_ocean_next_stage()
