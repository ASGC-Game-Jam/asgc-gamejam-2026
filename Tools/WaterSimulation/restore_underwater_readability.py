"""Restore underwater depth cues without the old full-screen blue wash."""
import unreal

editor=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert editor.get_editor_world().get_path_name().split('.')[0]=='/Game/ProjectAtlantis/Maps/POC/L_WaterSimulation_POC'
path='/Game/ProjectAtlantis/Maps/POC/District1/Materials/M_POC_UnderwaterDepth'
material=unreal.load_asset(path)
assert material
material.set_editor_property('material_domain',unreal.MaterialDomain.MD_POST_PROCESS)
unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
def node(cls,x,y):
    return unreal.MaterialEditingLibrary.create_material_expression(material,cls,x,y)
def scalar(name,value,x,y):
    n=node(unreal.MaterialExpressionScalarParameter,x,y)
    n.set_editor_property('parameter_name',name)
    n.set_editor_property('default_value',value)
    return n
def vector(name,value,x,y):
    n=node(unreal.MaterialExpressionVectorParameter,x,y)
    n.set_editor_property('parameter_name',name)
    n.set_editor_property('default_value',unreal.LinearColor(*value,1))
    return n
def link(a,out,b,pin):
    assert unreal.MaterialEditingLibrary.connect_material_expressions(a,out,b,pin)

scene=node(unreal.MaterialExpressionSceneTexture,-900,-250)
scene.set_editor_property('scene_texture_id',unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
# Very small animated, spatially varying refraction; HUD remains unaffected.
uv=node(unreal.MaterialExpressionScreenPosition,-1500,-500)
row=node(unreal.MaterialExpressionComponentMask,-1300,-500)
row.set_editor_property('r',False)
row.set_editor_property('g',True)
row.set_editor_property('b',False)
row.set_editor_property('a',False)
link(uv,'ViewportUV',row,'')
frequency=scalar('RippleFrequency',12,-1500,-300)
rows=node(unreal.MaterialExpressionMultiply,-1100,-450)
link(row,'',rows,'A')
link(frequency,'',rows,'B')
time=node(unreal.MaterialExpressionTime,-1500,-100)
speed=scalar('RippleSpeed',0.25,-1500,50)
clock=node(unreal.MaterialExpressionMultiply,-1300,-100)
link(time,'',clock,'A')
link(speed,'',clock,'B')
phase=node(unreal.MaterialExpressionAdd,-900,-500)
link(rows,'',phase,'A')
link(clock,'',phase,'B')
sine=node(unreal.MaterialExpressionSine,-700,-500)
link(phase,'',sine,'')
amplitude=scalar('RefractionAmount',0.0007,-900,-650)
offset=node(unreal.MaterialExpressionMultiply,-500,-500)
link(sine,'',offset,'A')
link(amplitude,'',offset,'B')
zero=node(unreal.MaterialExpressionConstant,-500,-650)
zero.set_editor_property('r',0.0)
xy=node(unreal.MaterialExpressionAppendVector,-300,-500)
link(offset,'',xy,'A')
link(zero,'',xy,'B')
distorted=node(unreal.MaterialExpressionAdd,-100,-500)
link(uv,'ViewportUV',distorted,'A')
link(xy,'',distorted,'B')
link(distorted,'',scene,'UVs')
# Use the viewport-aware SceneTexture lookup for depth as well as colour.
# SceneDepth's legacy unconnected UV path uses ScreenAlignedPosition and can
# sample a different rectangle when PIE occupies part of an editor buffer.
# UE 5.7 SceneTexture converts ViewportUV to the appropriate buffer UV itself.
depth_texture=node(unreal.MaterialExpressionSceneTexture,-1300,250)
depth_texture.set_editor_property('scene_texture_id',unreal.SceneTextureId.PPI_SCENE_DEPTH)
link(uv,'ViewportUV',depth_texture,'UVs')
depth=node(unreal.MaterialExpressionComponentMask,-900,250)
depth.set_editor_property('r',True)
depth.set_editor_property('g',False)
depth.set_editor_property('b',False)
depth.set_editor_property('a',False)
link(depth_texture,'Color',depth,'')
distance=scalar('VisibilityDistanceCm',3000,-900,450)
add=node(unreal.MaterialExpressionAdd,-650,350)
link(depth,'',add,'A')
link(distance,'',add,'B')
factor=node(unreal.MaterialExpressionDivide,-450,250)
link(depth,'',factor,'A')
link(add,'',factor,'B')
strength=scalar('ScatteringStrength',0.80,-450,450)
amount=node(unreal.MaterialExpressionMultiply,-200,250)
link(factor,'',amount,'A')
link(strength,'',amount,'B')
tint=vector('NearTransmission',(0.80,0.94,0.90),-900,-50)
transmission=node(unreal.MaterialExpressionMultiply,-650,-180)
rgb=node(unreal.MaterialExpressionComponentMask,-800,-250)
for channel in ['r','g','b']:
    rgb.set_editor_property(channel,True)
rgb.set_editor_property('a',False)
link(scene,'Color',rgb,'')
link(rgb,'',transmission,'A')
link(tint,'',transmission,'B')
water=vector('ScatteredLight',(0.012,0.070,0.061),-400,0)
blend=node(unreal.MaterialExpressionLinearInterpolate,100,0)
link(transmission,'',blend,'A')
link(water,'',blend,'B')
link(amount,'',blend,'Alpha')
assert unreal.MaterialEditingLibrary.connect_material_property(blend,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)

all_actors=actors.get_all_level_actors()
for actor in all_actors:
    label=actor.get_actor_label()
    if label=='POC_UnderwaterLook':
        actor.modify()
        settings=actor.get_editor_property('settings')
        settings.set_editor_property('weighted_blendables',unreal.WeightedBlendables(array=[unreal.WeightedBlendable(weight=1.0,object=material)]))
        settings.set_editor_property('override_scene_color_tint',False)
        actor.set_editor_property('settings',settings)
        actor.set_editor_property('priority',200)
    if label=='D1POC_TealAmbientFill':
        actor.modify()
        component=actor.get_component_by_class(unreal.PointLightComponent)
        component.modify()
        component.set_intensity(15000)
        component.set_light_color(unreal.LinearColor(0.4,0.65,0.6,1))
    if label=='POC_OceanSun':
        actor.modify()
        component=actor.get_component_by_class(unreal.DirectionalLightComponent)
        component.modify()
        component.set_intensity(0.08)
    if label=='POC_DeepFog':
        actor.modify()
        component=actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        component.modify()
        component.set_editor_property('fog_density',0.035)
        component.set_editor_property('fog_inscattering_luminance',unreal.LinearColor(0.012,0.055,0.048,1))

# Existing animated Niagara bubbles, placed only in submerged approaches.
bubbles=unreal.load_asset('/Game/ProjectAtlantis/Effects/Underwater/AdditionalBubbles/NS_CartoonBubbles_Translucent')
for i,position in enumerate([(20700,-650,3350),(22700,-1000,3200),(24200,300,3350),(25700,800,3550),(27800,800,3650),(20000,-900,4100)]):
    name=f'UWPOC_BubbleTrail_{i}'
    actor=next((a for a in all_actors if a.get_actor_label()==name),None)
    if not actor:
        actor=actors.spawn_actor_from_class(unreal.NiagaraActor,unreal.Vector(*position))
        actor.set_actor_label(name)
        actor.set_folder_path('Ocean POC/Underwater Atmosphere')
        actor.set_editor_property('is_spatially_loaded',False)
    actor.modify()
    component=actor.get_component_by_class(unreal.NiagaraComponent)
    component.modify()
    component.set_asset(bubbles)
    component.set_variable_float('User.SpawnRate',10.0)
    component.set_variable_float('User.Lifetime Min',8.0)
    component.set_variable_float('User.Lifetime Max',12.0)
    component.set_variable_float('User.Uniform Curve Scale',0.30)
    component.set_variable_float('User.Wobbleness',0.5)
    component.set_variable_linear_color('User.Color',unreal.LinearColor(0.35,0.55,0.5,0.7))
    actor.set_actor_scale3d(unreal.Vector(1,1,1))

assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('UNDERWATER_READABILITY_SAVED')
