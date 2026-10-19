"""Editor-only #288 asset migration. Acquire IMC_Default's LFS lock before running.

Copies only the existing fixed-state bindings, preserving every existing land mapping.
Can be rerun without duplicates. No swimming, Jump or breathing actions are changed.
"""
import unreal

ROOT = "/Game/ProjectAtlantis/Core/Engine/Input/"
land = unreal.load_asset(ROOT + "IMC_Default")
swim = unreal.load_asset(ROOT + "IMC_Swim")
assert land and swim, "Required authored input contexts are missing"
data = land.get_editor_property("default_key_mappings")
mappings = list(data.get_editor_property("mappings"))
source = swim.get_editor_property("default_key_mappings").get_editor_property("mappings")
names = {"IA_BallastDescend", "IA_BallastWander", "IA_BallastAscend"}
fixed = [mapping for mapping in source if mapping.action and mapping.action.get_name() in names]
assert len(fixed) == 6, "Expected keyboard/gamepad fixed-state bindings"
def identity(mapping):
    return (mapping.action.get_path_name(), str(mapping.key.get_editor_property("key_name")))
existing = {identity(mapping) for mapping in mappings if mapping.action}
added = 0
for mapping in fixed:
    assert not mapping.triggers and not mapping.modifiers, "Review instanced mapping objects before copying"
    if identity(mapping) not in existing:
        mappings.append(mapping)
        existing.add(identity(mapping))
        added += 1
if added:
    data.set_editor_property("mappings", mappings)
    land.set_editor_property("default_key_mappings", data)
    assert unreal.EditorAssetLibrary.save_loaded_asset(land, only_if_is_dirty=False)
assert all(identity(mapping) in existing for mapping in fixed)
unreal.log("LAND_BALLAST_MAPPINGS added=" + str(added) + " validated=6")
