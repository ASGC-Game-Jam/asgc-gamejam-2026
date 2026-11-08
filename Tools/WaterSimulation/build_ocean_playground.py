"""Rework the dedicated POC map as an ocean with playable underwater air pockets."""
import unreal

MAP = '/Game/ProjectAtlantis/Maps/POC/L_WaterSimulation_POC'
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = editor.get_editor_world()
assert world.get_path_name().split('.')[0] == MAP, 'Open the dedicated POC map first'

for actor in list(actors.get_all_level_actors()):
    if actor.get_actor_label().startswith('POC_') or isinstance(actor, (unreal.PhysicsVolume, unreal.PlayerStart)):
        actors.destroy_actor(actor)

def spawn(cls, label, xyz, rotation=None):
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*xyz), rotation or unreal.Rotator())
    assert actor, label
    actor.set_actor_label(label)
    actor.set_folder_path('Ocean POC')
    actor.set_editor_property('is_spatially_loaded', False)
    return actor

cube = unreal.load_asset('/Engine/BasicShapes/Cube')
sphere = unreal.load_asset('/Engine/BasicShapes/Sphere')

def mesh(label, xyz, size, asset=cube):
    actor = spawn(unreal.StaticMeshActor, label, xyz)
    actor.static_mesh_component.set_static_mesh(asset)
    actor.set_actor_scale3d(unreal.Vector(*(v / 100.0 for v in size)))
    return actor

mesh('POC_OceanFloor', (23500, 0, 2900), (32000, 32000, 200))
zone = spawn(unreal.WaterZone, 'POC_OceanZone', (23500, 0, 6500))
zone.set_editor_property('zone_extent', unreal.Vector2D(32000, 32000))
zone.set_editor_property('enable_local_only_tessellation', False)
ocean = spawn(unreal.WaterBodyOcean, 'POC_Ocean', (23500, 0, 6500))
body = ocean.get_component_by_class(unreal.WaterBodyOceanComponent)
body.set_editor_property('affects_landscape', False)
body.set_editor_property('ocean_extents', unreal.Vector2D(32000, 32000))
body.set_water_zone_override(zone)
# Move the ocean's island hole away from the playable area.
spline = ocean.get_component_by_class(unreal.WaterSplineComponent)
spline.set_spline_points([unreal.Vector(-12500,-12500,0), unreal.Vector(-12300,-12500,0), unreal.Vector(-12400,-12300,0)], unreal.SplineCoordinateSpace.LOCAL, True)
spline.k2_synchronize_and_broadcast_data_change()
body.set_editor_property('underwater_post_process_material', None)
water = spawn(unreal.AtlantisWaterVolume, 'POC_SwimmingOceanVolume', (23500, 0, 4750))
water.water_boundary.set_box_extent(unreal.Vector(16000,16000,1750), True)

def sign(label, text, xyz, size=55):
    actor = spawn(unreal.TextRenderActor, label, xyz, unreal.Rotator(yaw=-90))
    component = actor.get_component_by_class(unreal.TextRenderComponent)
    component.set_text(text)
    component.set_world_size(size)
    component.set_text_render_color(unreal.Color(100,245,220,255))

bubble_material = unreal.load_asset('/Game/ProjectAtlantis/MaterialLibrary/Effects/Underwater/MI_BubbleSphere')
for number, x, y, floor in [(1,22000,0,3500),(2,26500,1500,4300)]:
    center = (x,y,floor+400)
    air = spawn(unreal.AtlantisAirPocketVolume, f'POC_AirPocket_{number}', center)
    air.air_boundary.set_sphere_radius(850, True)
    mesh(f'POC_AirPocket_{number}_Floor', (x,y,floor-50), (1300,1300,100))
    shell = mesh(f'POC_AirPocket_{number}_Bubble', center, (1700,1700,1700), sphere)
    # Persist the profile: changing only CollisionEnabled is reset by BlockAll on reload.
    shell.static_mesh_component.set_collision_profile_name('NoCollision')
    if bubble_material:
        shell.static_mesh_component.set_material(0,bubble_material)
    light = spawn(unreal.PointLight, f'POC_AirPocket_{number}_Light', (x,y,floor+550))
    lc = light.get_component_by_class(unreal.PointLightComponent)
    lc.set_intensity(18000)
    lc.set_editor_property('attenuation_radius',1600)
    lc.set_light_color(unreal.LinearColor(0.4,1.0,0.7,1.0))
    sign(f'POC_AirPocket_{number}_Sign', f'AIR POCKET {number}\nWALK / BREATHE / REFILL OXYGEN\nSwim through the bubble to enter or leave', (x-500,y+350,floor+250), 45)

# A clear submerged route between the two pockets.
for i in range(8):
    x = 22000 + i * 600
    y = i * 200
    mesh(f'POC_RoutePillar_{i}', (x,y,3200), (90,90,400))

gm = unreal.load_class(None,'/Game/ProjectAtlantis/Core/GameModes/BP_AtlantisGameMode.BP_AtlantisGameMode_C')
assert gm
world.get_world_settings().set_editor_property('default_game_mode',gm)
spawn(unreal.PlayerStart,'POC_OceanPlayerStart',(20200,-1200,4400),unreal.Rotator(pitch=-10,yaw=30))
sun = spawn(unreal.DirectionalLight,'POC_OceanSun',(23500,0,9000),unreal.Rotator(pitch=-50,yaw=-30))
sunlight = sun.get_component_by_class(unreal.DirectionalLightComponent)
sunlight.set_intensity(5)
sunlight.set_editor_property('forward_shading_priority',2)

# Use the existing Atlantis traversal-driven underwater effect so it switches off in air.
pp = spawn(unreal.AtlantisUnderwaterPostProcessVolume,'POC_UnderwaterLook',(23500,0,4750))
pp.set_editor_property('unbound',True)
settings = pp.get_editor_property('settings')
settings.set_editor_property('override_scene_color_tint',False)
pp.set_editor_property('settings',settings)
sign('POC_OceanInstructions','DEEP OCEAN / FOLLOW THE LIT AIR POCKETS\nUse normal Atlantis controls and ballast\nAir pockets restore walking and oxygen', (20500,-800,4300),30)
editor.set_level_viewport_camera_info(unreal.Vector(18500,-5000,8500),unreal.Rotator(pitch=-25,yaw=35))
actors.set_selected_level_actors([water])
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('OCEAN_POC_BUILD_COMPLETE')

# Keep a rebuilt playground consistent with the deep-ocean art direction.
from pathlib import Path
exec((Path(unreal.Paths.project_dir()) / 'Tools/WaterSimulation/apply_deep_ocean_look.py').read_text())
exec((Path(unreal.Paths.project_dir()) / 'Tools/WaterSimulation/apply_district1_look.py').read_text())
exec((Path(unreal.Paths.project_dir()) / 'Tools/WaterSimulation/restore_underwater_readability.py').read_text())
