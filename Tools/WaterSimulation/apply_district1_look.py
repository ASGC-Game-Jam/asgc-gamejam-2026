"""District 1 reference pass: gilded stone pavilions, shell roofs and coral gardens.

Reference: FDS-004 District Design, District 1 boards on PDF pages 8-10.
Applies only to the dedicated ocean POC; gameplay volumes and access stay intact.
"""
import unreal
import math

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
editor = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
assert editor.get_editor_world().get_path_name().split('.')[0] == '/Game/ProjectAtlantis/Maps/POC/L_WaterSimulation_POC'
root = '/Game/ProjectAtlantis/Maps/POC/District1/Materials/'
master = unreal.load_asset(root+'M_POC_District1Palette')
assert master, 'Create the palette master first'
unreal.MaterialEditingLibrary.delete_all_material_expressions(master)
color = unreal.MaterialEditingLibrary.create_material_expression(master,unreal.MaterialExpressionVectorParameter,-500,0)
color.set_editor_property('parameter_name','Color')
color.set_editor_property('default_value',unreal.LinearColor(0.5,0.45,0.35,1))
unreal.MaterialEditingLibrary.connect_material_property(color,'',unreal.MaterialProperty.MP_BASE_COLOR)
for name, value, prop, y in [('Roughness',0.65,unreal.MaterialProperty.MP_ROUGHNESS,180),('Metallic',0.0,unreal.MaterialProperty.MP_METALLIC,300)]:
    node=unreal.MaterialEditingLibrary.create_material_expression(master,unreal.MaterialExpressionScalarParameter,-500,y)
    node.set_editor_property('parameter_name',name)
    node.set_editor_property('default_value',value)
    unreal.MaterialEditingLibrary.connect_material_property(node,'',prop)
glow=unreal.MaterialEditingLibrary.create_material_expression(master,unreal.MaterialExpressionScalarParameter,-500,450)
glow.set_editor_property('parameter_name','Glow')
glow.set_editor_property('default_value',0.0)
multiply=unreal.MaterialEditingLibrary.create_material_expression(master,unreal.MaterialExpressionMultiply,-200,450)
unreal.MaterialEditingLibrary.connect_material_expressions(color,'',multiply,'A')
unreal.MaterialEditingLibrary.connect_material_expressions(glow,'',multiply,'B')
unreal.MaterialEditingLibrary.connect_material_property(multiply,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
unreal.MaterialEditingLibrary.recompile_material(master)
unreal.EditorAssetLibrary.save_loaded_asset(master)
palette={
    'Limestone':((0.58,0.49,0.35),0.72,0,0),
    'Gold':((0.65,0.38,0.10),0.27,0.85,0),
    'Patina':((0.035,0.19,0.17),0.4,0.55,0),
    'Seabed':((0.065,0.10,0.085),0.95,0,0),
    'CoralRose':((0.50,0.075,0.16),0.6,0,0.1),
    'CoralViolet':((0.20,0.055,0.32),0.6,0,0.12),
    'Jade':((0.035,0.25,0.18),0.5,0,0.1),
    'AmberLight':((1.0,0.42,0.10),0.3,0.2,4),
    'GlassTeal':((0.035,0.36,0.32),0.18,0.25,0.6),
}
mats={}
for name,(rgb,roughness,metallic,emission) in palette.items():
    path=root+'MI_POC_D1_'+name
    material=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('MI_POC_D1_'+name,root.rstrip('/'),unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    unreal.MaterialEditingLibrary.set_material_instance_parent(material,master)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(material,'Color',unreal.LinearColor(*rgb,1))
    for parameter,value in [('Roughness',roughness),('Metallic',metallic),('Glow',emission)]:
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(material,parameter,value)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    mats[name]=material

for a in list(actors.get_all_level_actors()):
    if a.get_actor_label().startswith('D1POC_'):
        actors.destroy_actor(a)

cube=unreal.load_asset('/Engine/BasicShapes/Cube')
cylinder=unreal.load_asset('/Engine/BasicShapes/Cylinder')
sphere=unreal.load_asset('/Engine/BasicShapes/Sphere')
coral=unreal.load_asset('/Game/ProjectAtlantis/Art/D1_CityCenter_Blockout_Assets/SM_D1_Cityhub_Blockout_Coralcluster')
dome=unreal.load_asset('/Game/ProjectAtlantis/Art/D1_CityCenter_Blockout_Assets/SM_D1_CityHub_Blockout_Hemisphere')
sequence=0
def shape(name,position,size,material,asset=cube,rotation=None):
    global sequence
    sequence+=1
    a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*position),rotation or unreal.Rotator())
    a.set_actor_label('D1POC_'+name+'_'+str(sequence))
    a.set_folder_path('Ocean POC/District 1 Visuals')
    a.set_editor_property('is_spatially_loaded',False)
    a.static_mesh_component.set_static_mesh(asset)
    a.static_mesh_component.set_material(0,mats[material])
    a.static_mesh_component.set_collision_profile_name('NoCollision')
    a.set_actor_scale3d(unreal.Vector(*(s/100 for s in size)))
    return a

def rod(name,start,end,width,material):
    direction=unreal.Vector(*(end[i]-start[i] for i in range(3)))
    length=math.sqrt(sum((end[i]-start[i])**2 for i in range(3)))
    return shape(name,tuple((start[i]+end[i])/2 for i in range(3)),(width,width,length+2),material,cylinder,unreal.MathLibrary.make_rot_from_z(direction))

def arch(x,y,z,radius,material,width):
    for i in range(18):
        a=i*math.pi/18
        b=(i+1)*math.pi/18
        rod('NouveauArch',(x,y+radius*math.cos(a),z+radius*math.sin(a)),(x,y+radius*math.cos(b),z+radius*math.sin(b)),width,material)

def coral_fan(x,y,z,material,height=210):
    rod('CoralStem',(x,y,z),(x,y,z+height*0.6),25,material)
    for i in range(-2,3):
        end=(x+i*35,y+abs(i)*15,z+height-abs(i)*16)
        join=(x+i*12,y,z+height*0.45)
        rod('CoralBranch',join,end,19,material)
        shape('CoralTip',end,(24,24,30),material,sphere)
        fork=(end[0]+(-20 if i<0 else 20),end[1]+20,end[2]+35)
        rod('CoralFork',(end[0],end[1],end[2]-35),fork,14,material)
        shape('CoralBud',fork,(20,20,26),material,sphere)

for number,x,y,floor in [(1,22000,0,3500),(2,26500,1500,4300)]:
    # Classical stone-and-gold colonnades frame two open swimming entrances.
    for side in [-1,1]:
        for offset in [-480,480]:
            cx,cy=x+side*480,y+offset
            shape('Column',(cx,cy,floor+260),(80,80,500),'Limestone',cylinder)
            for height,diameter in [(30,125),(80,100),(470,100),(510,125)]:
                shape('GildedCapital',(cx,cy,floor+height),(diameter,diameter,24),'Gold',cylinder)
            for angle in range(0,360,60):
                r=math.radians(angle)
                shape('ColumnFlute',(cx+43*math.cos(r),cy+43*math.sin(r),floor+275),(10,10,330),'Gold',cylinder)
        arch(x+side*480,y,floor+250,480,'Limestone',70)
        arch(x+side*480,y,floor+250,498,'Gold',14)
    roof=shape('ShellCanopy',(x,y,floor+610),(65,65,27),'Patina',dome)
    # The authored hemisphere is 20 m wide; scale values above yield a 13 m canopy.
    for angle in range(0,360,45):
        phi=math.radians(angle)
        for segment in range(9):
            a=segment*math.pi/18
            b=(segment+1)*math.pi/18
            p=(x+650*math.sin(a)*math.cos(phi),y+650*math.sin(a)*math.sin(phi),floor+610+270*math.cos(a))
            q=(x+650*math.sin(b)*math.cos(phi),y+650*math.sin(b)*math.sin(phi),floor+610+270*math.cos(b))
            rod('ShellFinRib',p,q,14,'Gold')
    # Jewel-like windows, flowering coral planters and warm palace lanterns.
    for side in [-1,1]:
        for dx in [-250,250]:
            shape('Planter',(x+dx,y+side*560,floor+70),(250,140,100),'Limestone')
            shape('PlanterTrim',(x+dx,y+side*560,floor+125),(265,155,18),'Gold')
            coral_fan(x+dx,y+side*560,floor+130,'CoralRose' if side<0 else 'CoralViolet')
        shape('StainedGlass',(x,y+side*590,floor+350),(320,18,370),'GlassTeal')
        for dx in [-160,0,160]:
            shape('WindowMullion',(x+dx,y+side*602,floor+350),(10,12,400),'Gold')
        shape('WindowTransom',(x,y+side*602,floor+350),(340,12,12),'Gold')
    shape('PalaceLantern',(x,y,floor+580),(65,65,90),'AmberLight',sphere)

# Coral gardens on stone terraces outside the bubbles keep the swim-through clear.
for i in range(18):
    x=20500+i*420
    y=1000+600*math.sin(i*0.8)
    z=3020
    shape('GardenTerrace',(x,y,z),(360,300,100),'Limestone')
    coral_fan(x,y,z+55,['CoralRose','CoralViolet','Jade'][i%3],240+(i%3)*45)

# A modest palace silhouette gives the garden route a District 1 landmark.
for offset, height in [(-1400,1600),(0,2300),(1400,1600)]:
    x,y,z=24500+offset,5600,3000
    shape('PalaceStoneBase',(x,y,z+height/2),(1100,1200,height),'Limestone')
    shape('PalaceShellRoof',(x,y,z+height),(75,75,45),'Patina',dome)
    for side in [-1,1]:
        shape('PalacePilaster',(x+side*470,y-630,z+height/2),(120,120,height),'Gold',cylinder)
    for level in range(2):
        for dx in [-280,280]:
            shape('PalaceAmberWindow',(x+dx,y-610,z+350+level*550),(140,25,300),'AmberLight')

fill=actors.spawn_actor_from_class(unreal.PointLight,unreal.Vector(23500,-1800,5700))
fill.set_actor_label('D1POC_TealAmbientFill')
fill.set_folder_path('Ocean POC/District 1 Visuals')
fill.set_editor_property('is_spatially_loaded',False)
light=fill.get_component_by_class(unreal.PointLightComponent)
light.set_editor_property('intensity_units',unreal.LightUnits.LUMENS)
light.set_intensity(50000)
light.set_light_color(unreal.LinearColor(0.6,0.78,0.73,1))
light.set_editor_property('attenuation_radius',9000)
light.set_editor_property('cast_shadows',False)
light.set_editor_property('volumetric_scattering_intensity',0.25)

for actor in actors.get_all_level_actors():
    label=actor.get_actor_label()
    if label in ['POC_AirPocket_1_Floor','POC_AirPocket_2_Floor']:
        actor.modify()
        actor.static_mesh_component.modify()
        actor.static_mesh_component.set_material(0,mats['Limestone'])
    if label=='POC_OceanFloor':
        actor.modify()
        actor.static_mesh_component.modify()
        actor.static_mesh_component.set_material(0,mats['Seabed'])
    if label=='POC_DeepFog':
        actor.modify()
        fog=actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        fog.modify()
        fog.set_editor_property('fog_density',0.018)
        fog.set_editor_property('enable_volumetric_fog',False)
        fog.set_editor_property('fog_inscattering_luminance',unreal.LinearColor(0.018,0.065,0.055,1))
        fog.set_editor_property('volumetric_fog_albedo',unreal.Color(100,150,140,255))
    if label=='POC_OceanSun':
        actor.modify()
        actor.get_component_by_class(unreal.DirectionalLightComponent).set_intensity(0.5)
    if label=='POC_DeepExposure':
        actor.modify()
        settings=actor.get_editor_property('settings')
        settings.set_editor_property('auto_exposure_min_brightness',2)
        settings.set_editor_property('auto_exposure_max_brightness',2)
        actor.set_editor_property('settings',settings)
    if label.startswith('POC_AirPocket_') and label.endswith('_Sign'):
        actor.modify()
        c=actor.get_component_by_class(unreal.TextRenderComponent)
        c.modify()
        c.set_text('OXYGEN GARDEN\nDistrict I / Royal Conservatory')
        c.set_world_size(25)
        c.set_text_render_color(unreal.Color(235,195,115,255))
    if label=='POC_OceanInstructions':
        actor.modify()
        c=actor.get_component_by_class(unreal.TextRenderComponent)
        c.modify()
        c.set_text('DISTRICT I / OXYGEN GARDENS\nSwim through the open gilded arches\nBreathe / Walk / Restore oxygen')
        c.set_world_size(20)

assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.log('DISTRICT1_VISUAL_PASS_SAVED')
