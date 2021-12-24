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
        matrix = matrix_local.inverted() @ pbone.matrix @ pbone.bone.matrix_local.inverted() @ matrix_local
        matrix.transpose()
        f.write(struct.pack('ffff', *matrix[0]))
        f.write(struct.pack('ffff', *matrix[1]))
        f.write(struct.pack('ffff', *matrix[2]))
        f.write(struct.pack('ffff', *matrix[3]))
        
     
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
        
        
        f.write(struct.pack('i', len(mesh.vertices)))
        
        for vert in mesh.vertices:
            f.write(struct.pack('fff', *vert.co))
            f.write(struct.pack('fff', *vert.normal))
           
        f.write(struct.pack('i', len(mesh.loop_triangles)))
        
        flat_faces = []
        face_normals = []
        uv = []
        for face in mesh.loop_triangles:
            f.write(struct.pack('iii', *face.vertices))
            f.write(struct.pack('i', face.material_index))
            
            for loop_idx in face.loops:
                uv_cords = mesh.uv_layers.active.data[loop_idx].uv if mesh.uv_layers.active else (0, 0)
                uv.extend(uv_cords)
            if not face.use_smooth:
                flat_faces.append(face.index)
                face_normals.extend(face.normal)
        
        f.write(struct.pack('f' * len(uv), *uv))
        
        f.write(struct.pack('i', len(flat_faces)))
        f.write(struct.pack('i' * len(flat_faces), *flat_faces))
        f.write(struct.pack('f' * len(face_normals), *face_normals))
        
        f.write(struct.pack('i', len(mesh.materials)))
        
        def find_texture(socket):
            for link in socket.links:
                node = link.from_node
                print ("link", node.bl_static_type)
                
                if node.bl_static_type == "TEX_IMAGE":
                    texture = Path(node.image.filepath).name
                    return texture
                if node.bl_static_type == "MIX_RGB":
                    input = node.inputs["Color1"]
                    return find_texture(input)
                    
            
        for material in mesh.materials:
            #if material.name != 'lips':
            #    continue
            print("--------------------------")
            print("material", material.name, material.blend_method)
            texture = None
            col = material.diffuse_color
            if material.node_tree:
     
                for principled in material.node_tree.nodes:
                    if principled.bl_static_type != 'BSDF_PRINCIPLED':
                        continue

                    base_color = principled.inputs['Base Color']
                    value = base_color.default_value

                    col = [value[0], value[1], value[2], principled.inputs['Alpha'].default_value]
                    print("inter:",list(col))
                    
                    texture = find_texture(base_color)
                    if texture:
                        col[0] = 1.0
                        col[1] = 1.0
                        col[2] = 1.0
                        print(texture)
                        
                    if texture != None:
                        break #TODO objects with combined shaders
                       
            print(col)
            f.write(struct.pack('ffff', *col))
            if texture == None:
                f.write(struct.pack('i', 0)) #no texture flag
            else:
                f.write(struct.pack('i', len(texture)))
                f.write(bytes(texture, "ascii"))
            
        #f.close()
        #return {'FINISHED'}
        
        armature = obj.find_armature()
                
        if armature:
            pose = armature.pose
            world = armature.matrix_world
            
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
                   
                    
            if export_anim_setting:
                f.write(struct.pack('i', context.scene.frame_end))
                for frame in range(context.scene.frame_end):
                    context.scene.frame_set(frame)
                    write_frame_data(used_bones, obj.matrix_local, f)

            else:
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