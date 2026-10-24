"""Export the saved Blender asset and its exact optical mask for the game.

Run using Blender --background --disable-autoexec --python export_game.py.
The source file is read only. Posed modifiers are evaluated once at export;
the game uploads the resulting static geometry once per graphics context.
"""
import bpy,bmesh,struct,json,hashlib,math,numpy as np
from pathlib import Path
from mathutils import Vector

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
bpy.ops.wm.open_mainfile(filepath=str(HERE/'lightcycle.blend'),use_scripts=False)
scene=bpy.context.scene
assert scene.get('black_finish_and_seam_repair')
bike=bpy.data.objects['back.001'];rider=bpy.data.objects['model']

# Identify complete wheel components rather than rotating the fairings or pegs.
bm=bmesh.new();bm.from_mesh(bike.data);remaining=set(bm.verts);components=[]
while remaining:
    seed=remaining.pop();todo=[seed];vs={seed}
    while todo:
        v=todo.pop()
        for e in v.link_edges:
            w=e.other_vert(v)
            if w in remaining:remaining.remove(w);vs.add(w);todo.append(w)
    ps=[bike.matrix_world@v.co for v in vs]
    low=Vector(tuple(min(p[i] for p in ps) for i in range(3)))
    high=Vector(tuple(max(p[i] for p in ps) for i in range(3)))
    faces={f for v in vs for f in v.link_faces}
    components.append((sorted(v.index for v in vs),low,high,{f.material_index for f in faces}))
bm.free()
tyres=sorted([p for p in components if p[3]=={3}],key=lambda p:p[1].x)
assert len(tyres)==2
axles=[(p[1]+p[2])*.5 for p in tyres]
# Preserve the renderer's established wheelbase, origin and movement envelope.
scale=3.0/(axles[1].x-axles[0].x)
offset=-1.45-axles[0].x*scale
floor=min(p[1].z for p in tyres)
centres=[((p.x*scale+offset),0,(p.z-floor)*scale) for p in axles]
motion=bike.data.attributes.new('Export wheel group','INT','POINT')
wheel_parts=[]
for ids,lo,hi,mats in components:
    if mats not in ({1},{2},{3}):continue
    if hi.x-lo.x<.50 or hi.z-lo.z<.50:continue
    centre=(lo+hi)*.5
    which=min(range(2),key=lambda i:abs(centre.x-axles[i].x))
    if abs(centre.x-axles[which].x)>.03 or abs(centre.z-axles[which].z)>.03:continue
    for i in ids:motion.data[i].value=which+1
    wheel_parts.append({'vertices':len(ids),'wheel':which,'materials':sorted(mats)})
assert len(wheel_parts)==6,wheel_parts

# Bake only emission coverage; colour remains a runtime player/team property.
palette=bpy.data.node_groups['Cycle and trail colour']
palette.nodes['Shared colour — bike, suit and trail'].outputs[0].default_value=(1,1,1,1)
palette.nodes['Shared light intensity'].outputs[0].default_value=1
image=bpy.data.images.new('Game optical coverage',width=2048,height=2048,alpha=False)
image.colorspace_settings.name='Non-Color'
material=bpy.data.materials['gloss.001'];node=material.node_tree.nodes.new('ShaderNodeTexImage');node.image=image
material.node_tree.nodes.active=node
bpy.ops.object.select_all(action='DESELECT');rider.select_set(True);bpy.context.view_layer.objects.active=rider
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=1
scene.render.threads_mode='FIXED';scene.render.threads=4
scene.render.bake.margin=12;scene.render.bake.use_clear=True
bpy.ops.object.bake(type='EMIT')
image.filepath_raw=str(ROOT/'textures/lightcycle_suit_mask.png');image.file_format='PNG';image.save()
pixels=np.array(image.pixels[:],dtype=np.float32).reshape(2048,2048,4)
assert pixels[:,:,:3].max()>.99 and np.mean(pixels[:,:,0]>.5)>.01

parts=[[[] for _ in range(10)] for _ in range(4)] # body, rear wheel, front wheel, rider
material_ids={'gloss':0,'light':7,'styeel':2,'wheel':3,'gloss.001':5}
stats={}
for name in ['back.001','model','Circle.008']:
    obj=bpy.data.objects[name]
    ev=obj.evaluated_get(bpy.context.evaluated_depsgraph_get());me=ev.to_mesh(preserve_all_data_layers=True,depsgraph=bpy.context.evaluated_depsgraph_get())
    me.calc_loop_triangles()
    uv=me.uv_layers.get('UVTex')
    normal_matrix=obj.matrix_world.to_3x3().inverted().transposed()
    attr=me.attributes.get('Export wheel group')
    housing=me.attributes.get('Rear optical surface')
    reference=me.attributes.get('Bike optical reference')
    mirror_centre=(bike.matrix_world@Vector((0,0,0))).y
    for tri in me.loop_triangles:
        mat=material_ids[obj.data.materials[tri.material_index].name]
        group=3 if name!='back.001' else int(round(sum(attr.data[i].value for i in tri.vertices)/3))
        assert 0<=group<=3
        for vertex_id,loop_id in zip(tri.vertices,tri.loops):
            p=obj.matrix_world@me.vertices[vertex_id].co
            world_z=p.z
            p=Vector((p.x*scale+offset,p.y*scale,(p.z-floor)*scale))
            if group in (1,2):p-=Vector(centres[group-1])
            n=(normal_matrix@me.corner_normals[loop_id].vector).normalized()
            # rSurface uploads SDL's top-down rows; the existing OBJ loader
            # uses this same V flip when converting authored texture UVs.
            original_uv=uv.data[loop_id].uv[:] if uv else (0,0)
            tex=(original_uv[0],1-original_uv[1])
            rear=housing.data[vertex_id].value if housing else 0
            # This scalar is copied through Mirror with the original side's
            # position. Its absolute distance to the centre is symmetric.
            distance=abs(reference.data[vertex_id].vector.y-mirror_centre) if reference else 0
            zfade=max(0,min(1,(world_z-.035)/.020));zfade=zfade*zfade*(3-2*zfade)
            parts[group][mat].append((*p,*n,*tex,rear*zfade,distance))
    stats[name]=len(me.loop_triangles);ev.to_mesh_clear()
path=ROOT/'models/lightcycle-authored.bin'
profile=[]
study=bpy.data.objects['TRAIL | Static material review']
for v in study.data.vertices[:33]:
    profile.append(((v.co.z-floor)*scale,v.co.x*scale+offset))
header='// Generated from lightcycle.blend by export_game.py. Visual attachment only.\n'
header+='#ifndef ARMAGETRON_LIGHTCYCLE_PROFILE_H\n#define ARMAGETRON_LIGHTCYCLE_PROFILE_H\n'
header+='namespace gLightcycleProfile {\n'
header+='constexpr float RearExtent=%.9ff;\n'%min(x for z,x in profile)
header+='constexpr float Radii[2]={%.9ff,%.9ff};\n'%tuple((p[2].z-p[1].z)*.5*scale for p in tyres)
header+='constexpr float Rear[33][2]={\n'+''.join('    {%.9ff,%.9ff},\n'%p for p in profile)+'};\n}\n#endif\n'
(ROOT/'src/tron/gLightcycleProfile.h').write_text(header)
with path.open('wb') as f:
    f.write(b'LCMESH1\0');f.write(struct.pack('<II',4,10))
    f.write(struct.pack('<6f',*(v for p in centres for v in p)))
    for group in parts:
        for part in group:
            f.write(struct.pack('<I',len(part)))
            if part:
                data=np.asarray(part,dtype='<f4');assert np.isfinite(data).all()
                assert np.allclose(np.linalg.norm(data[:,3:6],axis=1),1,atol=.001)
                f.write(data.tobytes())
audit={'source_blend_sha256':hashlib.sha256((HERE/'lightcycle.blend').read_bytes()).hexdigest(),
       'mesh_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
       'mask_sha256':hashlib.sha256((ROOT/'textures/lightcycle_suit_mask.png').read_bytes()).hexdigest(),
       'triangles':stats,'total_triangles':sum(stats.values()),'bytes':path.stat().st_size,
       'wheel_centres':centres,'uniform_scale':scale,'x_offset':offset,'source_floor':floor,
       'rear_profile':profile,
       'rotating_wheel_components':wheel_parts,'pose_frame':scene.frame_current,
       'optical_mask':'Baked scalar emission coverage; player/team colour is applied at runtime',
       'parts_vertices':[[len(p) for p in g] for g in parts]}
(HERE/'game-export.json').write_text(json.dumps(audit,indent=2)+'\n')
print('AUTHORED_GAME_EXPORT',json.dumps(audit),flush=True)
