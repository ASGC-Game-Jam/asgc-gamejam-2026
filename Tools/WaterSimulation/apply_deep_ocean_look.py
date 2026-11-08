"""Apply the deep-ocean lighting treatment only to the dedicated POC map."""
import unreal

editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert editor.get_editor_world().get_path_name().split('.')[0] == '/Game/ProjectAtlantis/Maps/POC/L_WaterSimulation_POC'
all_actors = actors.get_all_level_actors()
surface_z = 106500.0

for actor in all_actors:
    label = actor.get_actor_label()
    if label=='POC_OceanInstructions':
        actor.modify()
        text = actor.get_component_by_class(unreal.TextRenderComponent)
        text.modify()
        text.set_text('DEEP OCEAN / FOLLOW THE LIT AIR POCKETS\nUse normal Atlantis controls and ballast\nAir pockets restore walking and oxygen')
        text.set_world_size(30)
    if label in ['POC_Ocean', 'POC_OceanZone']:
        actor.modify()
        location = actor.get_actor_location()
        actor.set_actor_location(unreal.Vector(location.x, location.y, surface_z), False, False)
    if label == 'POC_SwimmingOceanVolume':
        actor.modify()
        actor.water_boundary.modify()
        actor.set_actor_location(unreal.Vector(23500,0,(surface_z+3000)/2), False, False)
        actor.water_boundary.set_box_extent(unreal.Vector(16000,16000,(surface_z-3000)/2),True)
    if isinstance(actor, unreal.PostProcessVolume):
        actor.modify()
        # The copied tutorial volume re-enables itself at BeginPlay and carries
        # MPPI_Underwater_1, a blue SceneDepth post-process material.
        settings = (unreal.PostProcessSettings() if label in
                    ['PPV_Tutorial_Underwater','POC_UnderwaterLook']
                    else actor.get_editor_property('settings'))
        settings.set_editor_property('override_scene_color_tint',False)
        settings.set_editor_property('scene_color_tint',unreal.LinearColor(1,1,1,1))
        actor.set_editor_property('settings',settings)
    if isinstance(actor, unreal.DirectionalLight):
        actor.modify()
        component = actor.get_component_by_class(unreal.DirectionalLightComponent)
        component.modify()
        component.set_editor_property('visible',label=='POC_OceanSun')
        if label=='POC_OceanSun':
            component.set_intensity(0.02)
            component.set_light_color(unreal.LinearColor(0.85,0.9,0.85,1))
    if isinstance(actor, unreal.SkyLight):
        actor.modify()
        actor.get_component_by_class(unreal.SkyLightComponent).set_editor_property('visible',False)
    if isinstance(actor, (unreal.SkyAtmosphere,unreal.VolumetricCloud)) or label=='SM_SkySphere':
        actor.modify()
        actor.set_actor_hidden_in_game(True)
        actor.set_is_temporarily_hidden_in_editor(True)
    if isinstance(actor,unreal.ExponentialHeightFog) and label!='POC_DeepFog':
        actor.modify()
        actor.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property('visible',False)
    if label in ['POC_AirPocket_1_Light','POC_AirPocket_2_Light']:
        actor.modify()
        component = actor.get_component_by_class(unreal.PointLightComponent)
        component.modify()
        component.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
        component.set_intensity(8000)
        component.set_light_color(unreal.LinearColor(1.0,0.8,0.55,1))
        component.set_editor_property('attenuation_radius',2200)

def get_or_spawn(cls, label, location):
    actor = next((a for a in all_actors if a.get_actor_label()==label),None)
    if not actor:
        actor = actors.spawn_actor_from_class(cls,unreal.Vector(*location))
        actor.set_actor_label(label)
        actor.set_folder_path('Ocean POC')
        actor.set_editor_property('is_spatially_loaded',False)
    actor.modify()
    return actor

fog = get_or_spawn(unreal.ExponentialHeightFog,'POC_DeepFog',(23500,0,4500))
fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
fc.modify()
for key,value in {
    'fog_density':0.015,
    'fog_height_falloff':0.001,
    'fog_inscattering_luminance':unreal.LinearColor(0.0005,0.0006,0.0005,1),
    'fog_max_opacity':1.0,
    'start_distance':100.0,
    'enable_volumetric_fog':True,
    'volumetric_fog_distance':12000.0,
    'volumetric_fog_albedo':unreal.Color(90,100,90,255),
    'volumetric_fog_emissive':unreal.LinearColor(0,0,0,1),
}.items():
    fc.set_editor_property(key,value)

exposure = get_or_spawn(unreal.PostProcessVolume,'POC_DeepExposure',(23500,0,4500))
exposure.set_editor_property('unbound',True)
exposure.set_editor_property('priority',100)
settings = exposure.get_editor_property('settings')
for key,value in {
    'override_auto_exposure_min_brightness':True,
    'override_auto_exposure_max_brightness':True,
    'auto_exposure_min_brightness':2.5,
    'auto_exposure_max_brightness':2.5,
    'override_bloom_intensity':True,
    'bloom_intensity':0.1,
}.items():
    settings.set_editor_property(key,value)
exposure.set_editor_property('settings',settings)

# Dim pools of warm light make the route readable without a daylight ocean.
for i, location in enumerate([(20200,-1200,4500),(23200,400,4000),(25000,1000,4400)]):
    lamp = get_or_spawn(unreal.PointLight,f'POC_DeepRouteLight_{i}',location)
    component = lamp.get_component_by_class(unreal.PointLightComponent)
    component.modify()
    component.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
    component.set_intensity(2000)
    component.set_light_color(unreal.LinearColor(0.85,0.88,0.75,1))
    component.set_editor_property('attenuation_radius',2200)

editor.set_level_viewport_camera_info(unreal.Vector(19000,-3000,5200),unreal.Rotator(pitch=-12,yaw=35))
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('DEEP_OCEAN_LOOK_SAVED')
