# See https://docs.blender.org/api/blender_python_api_2_78_0/info_tutorial_addon.html for more info

bl_info = {
    "name": "Defold Mesh Binary Export",
    "author": "",
    "version": (1, 0),
    "blender": (3, 0, 0),
    "location": "File > Export > Defold Binary Mesh (.bin)",
    "description": "Export to Defold .mesh format",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"
}

import bpy, sys, struct
from pathlib import Path

def write_frame_data(bones, matrix_local, f):
    for pbone in bones:
        #matrix = matrix_local.inverted() @ pbone.matrix @ pbone.bone.matrix_local.inverted() @ matrix_local
        f.write(struct.pack('ffff', *pbone.matrix[0]))
        f.write(struct.pack('ffff', *pbone.matrix[1]))
        f.write(struct.pack('ffff', *pbone.matrix[2]))
     
        
     
def optimize(bones, vertices, vertex_groups, limit_per_vertex):
          
    def sort_weights(vg):
        return -vg.weight
               
    bones_map = {bone.name: False for bone in bones}
    vertex_groups_per_vertex = []
    for vert in vertices:
        fixed_groups = []
        for wgrp in vert.groups:
            group = vertex_groups[wgrp.group]
            if wgrp.weight > 0 and group.name in bones_map:
                fixed_groups.append(wgrp)
                bones_map[group.name] = True
            
        fixed_groups.sort(key = sort_weights)
        fixed_groups = fixed_groups[:limit_per_vertex]
        total = 0
        for vg in fixed_groups:
            total = total + vg.weight
        for vg in fixed_groups:
            vg.weight = vg.weight / total
        vertex_groups_per_vertex.append(fixed_groups)
            
        used_bones = []
        for bone in bones:
            if bones_map[bone.name]:
                used_bones.append(bone)
    return used_bones, vertex_groups_per_vertex

def write_some_data(context, filepath, export_anim_setting, export_hidden_settings):
    
    f = open(filepath, 'wb')
     
    for obj in context.scene.objects:
        if obj.type != 'MESH':
            continue
        
        if obj.mode == 'EDIT':
            obj.mode_set(mode='OBJECT', toggle=False)
            
        mesh = obj.data
        
        mesh.calc_loop_triangles()
        mesh.calc_normals_split()
        
        
        if len(mesh.loop_triangles) == 0:
            continue
        
        if obj.hide_get() and not export_hidden_settings:
            continue
        
        print(obj.name)
        
        f.write(struct.pack('i', len(obj.name)))
        f.write(bytes(obj.name, "ascii"))
       
        if obj.parent:
            f.write(struct.pack('i', len(obj.parent.name)))
            f.write(bytes(obj.parent.name, "ascii"))
        else:
            f.write(struct.pack('i', 0)) #no parent flag
                        
        f.write(struct.pack('fff', *obj.location))
        f.write(struct.pack('fff', *obj.rotation_euler))
        f.write(struct.pack('fff', *obj.scale))
        
        #keep world transform in case we won't export parent object (e.g. ARMATURE)
        (translation, rotation, scale) = obj.matrix_world.decompose()
        f.write(struct.pack('fff', *translation))
        f.write(struct.pack('fff', *rotation.to_euler()))
        f.write(struct.pack('fff', *scale))
        
        #---------------------read-materials-----------------------
        def find_nodes_to(socket, type):
            for link in socket.links:
                node = link.from_node
                if node.bl_static_type == type:
                    return [node]
                else:
                    for input in node.inputs:
                        n = find_nodes_to(input, type)
                        if n:
                            n.append(node)
                            return n
            return None
        
        def find_node(socket, type):
            nodes = find_nodes_to(socket, type)
            if nodes:
                #print([n.bl_static_type for n in nodes])
                return nodes[0]
            return None
                
        def find_texture(socket):
            node = find_node(socket, 'TEX_IMAGE')
            if node != None:
                texture = Path(node.image.filepath).name
                return texture
                
            return None
        
        materials = []
        for m in mesh.materials:
            #print("--------------------------")
            #print("material", m.name, m.blend_method)
            
            material = {'col':  m.diffuse_color, 'blend': m.blend_method, 'spec_power': 0.0, 'normal_strength': 1.0}
            materials.append(material)
            
            if m.node_tree:
     
                for principled in m.node_tree.nodes:
                    if principled.bl_static_type != 'BSDF_PRINCIPLED':
                        continue

                    specular = principled.inputs['Specular']
                    material['spec_power'] = specular.default_value
                    material['specular'] = find_texture(specular)
                   
                    if material.get('specular'):
                        material['specular_invert'] = 1 if find_node(specular, 'INVERT') else 0 
                        
                    roughness = principled.inputs['Roughness']
                    material['roughness'] = roughness.default_value
                    material['roughness_tex'] = find_texture(roughness)
                   
                    normal_map = find_node(principled.inputs['Normal'], 'NORMAL_MAP')
                    if normal_map:
                        material['normal'] = find_texture(normal_map.inputs['Color'])
                        material['normal_strength'] = normal_map.inputs['Strength'].default_value
                    
                    base_color = principled.inputs['Base Color']
                    value = base_color.default_value

                    material['col'] = [value[0], value[1], value[2], principled.inputs['Alpha'].default_value]
                    
                    material['texture'] = find_texture(base_color)
                    
                    if material.get('texture') != None:
                        #print(material['texture'])
                        break #TODO objects with combined shaders
        
        #---------------------write-geometry------------------------
        
        f.write(struct.pack('i', len(mesh.vertices)))
        
        for vert in mesh.vertices:
                f.write(struct.pack('fff', *vert.co))
                f.write(struct.pack('fff', *vert.normal))
                
        shapes = []
        if mesh.shape_keys and len(mesh.shape_keys.key_blocks) > 0:
            for shape in mesh.shape_keys.key_blocks:
                s = {'name': shape.name, 'deltas': []}
                normals = shape.normals_vertex_get()
                for i in range(len(shape.data)):
                    vert  = mesh.vertices[i]
                    if (shape.data[i].co - vert.co).length > 0.001:
                        dpos = shape.data[i].co - vert.co
                        s['deltas'].append({'idx': i, 'p': dpos, 'n': (normals[i*3] - vert.normal.x, normals[i*3 + 1] - vert.normal.y, normals[i*3 + 2]- vert.normal.z)})
                   
                if len(s['deltas']) > 0:
                    shapes.append(s)
            
        f.write(struct.pack('i', len(shapes)))
        for shape in shapes:
             f.write(struct.pack('i', len(shape['name'])))
             f.write(bytes(shape['name'], "ascii"))
             f.write(struct.pack('i', len(shape['deltas'])))
             for vert in shape['deltas']:
                 f.write(struct.pack('i', vert['idx']))
                 f.write(struct.pack('fff', *vert['p']))
                 f.write(struct.pack('fff', *vert['n']))
           
        f.write(struct.pack('i', len(mesh.loop_triangles)))
        
        uv = []
        for face in mesh.loop_triangles:
            f.write(struct.pack('iii', *face.vertices))
            f.write(struct.pack('i', face.material_index))
            
            for loop_idx in face.loops:
                uv_cords = mesh.uv_layers.active.data[loop_idx].uv if mesh.uv_layers.active else (0, 0)
                uv.extend(uv_cords)
                
            if not face.use_smooth:
                f.write(struct.pack('i', 1))
                f.write(struct.pack('fff', *face.normal))
            else:
                 f.write(struct.pack('i', 0))
                
        f.write(struct.pack('f' * len(uv), *uv))
        
        f.write(struct.pack('i', len(mesh.materials)))
        
        #---------------------write-materials------------------------
        for material in materials:
            
            f.write(struct.pack('i', 0 if material['blend'] == 'OPAQUE' else 1))
            f.write(struct.pack('ffff', *material['col']))
            f.write(struct.pack('f', material['spec_power']))
            f.write(struct.pack('f', material['roughness']))
            
            if material.get('texture') == None:
                f.write(struct.pack('i', 0)) #no texture flag
            else:
                f.write(struct.pack('i', len(material['texture'])))
                f.write(bytes(material['texture'], "ascii"))
                
            if material.get('normal') == None:
                f.write(struct.pack('i', 0)) #no normal texture flag
            else:
                f.write(struct.pack('i', len(material['normal'])))
                f.write(bytes(material['normal'], "ascii"))
                f.write(struct.pack('f', material['normal_strength']))
                
            if material.get('specular') == None:
                f.write(struct.pack('i', 0)) #no texture flag
            else:
                f.write(struct.pack('i', len(material['specular'])))
                f.write(bytes(material['specular'], "ascii"))
                f.write(struct.pack('i', material['specular_invert']))
                
            if material.get('roughness_tex') == None:
                f.write(struct.pack('i', 0)) #no roughbess texture flag
            else:
                f.write(struct.pack('i', len(material['roughness_tex'])))
                f.write(bytes(material['roughness_tex'], "ascii"))
                        
            
        #f.close()
        #return {'FINISHED'}
        
        #---------------------write-bones------------------------
         
        armature = obj.find_armature()
                
        if armature:
            pose = armature.pose
            
            #optimizing bones, checking empty bones, etc
            #set limit to 4 bones per vertex
            
            used_bones, vertex_groups_per_vertex = optimize(pose.bones, mesh.vertices, obj.vertex_groups, 4)  
            
            print("USED BONES: ", len(used_bones))
            bones_map = {bone.name: i for i, bone in enumerate(used_bones)}
            f.write(struct.pack('i', len(used_bones)))
                
            for groups in vertex_groups_per_vertex:
                f.write(struct.pack('i', len(groups)))
                for wgrp in groups:
                    group = obj.vertex_groups[wgrp.group]
                    bone_idx = bones_map[group.name]
                    f.write(struct.pack('i', bone_idx))
                    f.write(struct.pack('f', wgrp.weight))
                   
            for pbone in used_bones:
                matrix = pbone.bone.matrix_local.inverted()
                f.write(struct.pack('ffff', *matrix[0]))
                f.write(struct.pack('ffff', *matrix[1]))
                f.write(struct.pack('ffff', *matrix[2]))
        
            if export_anim_setting:
                f.write(struct.pack('i', context.scene.frame_end))
                for frame in range(context.scene.frame_end):
                    context.scene.frame_set(frame)
                    write_frame_data(used_bones, obj.matrix_local, f)

            else:
                world = armature.matrix_world
                f.write(struct.pack('i', 1)) #single frame flag
                write_frame_data(used_bones, obj.matrix_local, f)
        else:
            f.write(struct.pack('i', 0)) #no bones flag

    f.close()

    return {'FINISHED'}


# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.props import StringProperty, BoolProperty, EnumProperty
from bpy.types import Operator

class DefoldExport(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_mesh.defold_binary"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export binary"
    bl_description  = "Export Defold binary mesh data"

    # ExportHelper mixin class uses this
    filename_ext = ".bin"

    filter_glob: StringProperty(
        default="*.bin",
        options={'HIDDEN'},
        maxlen=255,  # Max internal buffer length, longer would be clamped.
    )
    
    export_anim: BoolProperty(
        name="Export animations",
        description="Only for armatures",
        default=False,
    )
    
    export_hidden: BoolProperty(
        name="Export hidden meshes",
        description="",
        default=False,
    )


    def execute(self, context):
        return write_some_data(context, self.filepath, self.export_anim, self.export_hidden)


# Only needed if you want to add into a dynamic menu
def menu_func_export(self, context):
    self.layout.operator(DefoldExport.bl_idname, text="Defold Binary Mesh (.bin)")

# Register and add to the "file selector" menu (required to use F3 search "Text Export Operator" for quick access)
def register():
    bpy.utils.register_class(DefoldExport)
    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)


def unregister():
    bpy.utils.unregister_class(DefoldExport)
    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)


if __name__ == "__main__":
    register()

    # test call
    bpy.ops.export_mesh.defold_binary('INVOKE_DEFAULT')