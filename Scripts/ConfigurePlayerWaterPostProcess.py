"""Functional #289 migration, requiring LFS locks on the map and original volume package.

Only moves the existing underwater material to the native per-view gate. Original
volume settings, geometry, material asset and unrelated volumes are preserved.
Use -WaterGateVerifyOnly to reopen and validate without saving.
"""
import json
from pathlib import Path
import unreal

MAP = "/Game/ProjectAtlantis/Maps/Tutorial/L_Tutorial_Main"
MATERIAL = "/Game/ProjectAtlantis/MaterialLibrary/Post-Process/SceneDepth/MPPI_Underwater_1.MPPI_Underwater_1"
SOURCE_NAME = "PostProcessVolume_UAID_DC567BBFCB4EA20803_1583181791"
verify_only = "-WaterGateVerifyOnly" in unreal.SystemLibrary.get_command_line()
editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert editor.load_level(MAP)
descs = unreal.WorldPartitionBlueprintLibrary.get_actor_descs()
source_desc = next(desc for desc in descs if str(desc.name) == SOURCE_NAME)
unreal.WorldPartitionBlueprintLibrary.load_actors([source_desc.guid])
actors = actors_api.get_all_level_actors()
source = next(actor for actor in actors if actor.get_name() == SOURCE_NAME)
override_flags = [name for name in dir(source.settings) if name.startswith("override_") and getattr(source.settings, name)]
assert not override_flags, "Review additional authored water settings before using the material-only migration"
material = unreal.load_asset(MATERIAL)
gate_class = unreal.load_class(None, "/Script/ProjectAtlantis.AtlantisPlayerWaterPostProcess")
assert gate_class and material
gates = [actor for actor in actors if actor.get_class() == gate_class]
assert len(gates) <= 1
if not gates:
    assert not verify_only, "Player water gate is not authored"
    settings = source.get_editor_property("settings")
    blendables = settings.get_editor_property("weighted_blendables")
    entries = list(blendables.get_editor_property("array"))
    matches = [entry for entry in entries if entry.object == material]
    assert len(matches) == 1 and matches[0].weight > 0
    original_weight = matches[0].weight
    gate = actors_api.spawn_actor_from_class(gate_class, source.get_actor_location())
    assert gate
    gate.set_actor_label("PP_PlayerWater")
    gate.set_editor_property("source_volume", source)
    gate.set_editor_property("underwater_material", material)
    gate.set_editor_property("material_weight", original_weight)
    gate.set_editor_property("is_spatially_loaded", False)
    # The always-loaded gate keeps a hard reference to the original authored region.
    source.set_editor_property("is_spatially_loaded", False)
    gate.post_process.set_editor_property("priority", source.priority)
    gate.post_process.set_editor_property("blend_weight", 1.0)
    blendables.set_editor_property("array", [entry for entry in entries if entry.object != material])
    settings.set_editor_property("weighted_blendables", blendables)
    source.set_editor_property("settings", settings)
    assert unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
else:
    gate = gates[0]

assert gate.source_volume == source and gate.underwater_material == material
assert gate.post_process.unbound and gate.post_process.enabled
assert gate.post_process.priority == source.priority
assert not source.unbound, "Original bounded region must stay bounded"
assert not source.get_editor_property("is_spatially_loaded")
entries = source.settings.weighted_blendables.array
assert all(entry.object != material for entry in entries), "Camera-based material entry remains"
assert len(gate.post_process.settings.weighted_blendables.array) == 1
assert gate.post_process.settings.weighted_blendables.array[0].object == gate
generic = [actor for actor in actors if isinstance(actor, unreal.PostProcessVolume) and actor.unbound]
assert generic and all(actor.enabled for actor in generic), "Unrelated unbound volumes must remain enabled"
report = {"map": MAP, "source_actor": source.get_path_name(), "source_package": str(source_desc.actor_package),
          "gate": gate.get_path_name(), "material": MATERIAL, "priority": source.priority,
          "other_unbound_volumes": [actor.get_path_name() for actor in generic], "verify_only": verify_only,
          "source_enabled_override_flags": override_flags}
out = Path(unreal.Paths.project_saved_dir()) / "Logs" / "PlayerWaterAsset574.json"
out.write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("PLAYER_WATER_GATE_ASSET_VALIDATED " + str(out))
