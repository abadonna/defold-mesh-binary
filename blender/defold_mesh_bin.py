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

def write_frame_data(pose, matrix_local, f):
    for pbone in pose.bones:
        matrix = matrix_local.inverted() @ pbone.matrix @ pbone.bone.matrix_local.inverted() @ matrix_local
        matrix.transpose()
        f.write(struct.pack('ffff', *matrix[0]))
        f.write(struct.pack('ffff', *matrix[1]))
        f.write(struct.pack('ffff', *matrix[2]))
        f.write(struct.pack('ffff', *matrix[3]))

def write_some_data(context, filepath, export_anim_setting):
    
    f = open(filepath, 'wb')
     
    for obj in context.scene.objects:
        if obj.type != 'MESH':
            continue
        
        if obj.mode == 'EDIT':
            obj.mode_set(mode='OBJECT', toggle=False)
        
        mesh = obj.data
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
        
                 
        mesh.calc_loop_triangles()
        mesh.calc_normals_split()
        
        f.write(struct.pack('i', len(mesh.vertices)))
        
        for vert in mesh.vertices:
            f.write(struct.pack('fff', *vert.co))
            f.write(struct.pack('fff', *vert.normal))
           
        #vertices = [(vert.co.x, vert.co.y, vert.co.z) for vert in mesh.vertices]
        #normals = [(vert.normal.x, vert.normal.y, vert.normal.z) for vert in mesh.vertices]
        #faces = [(face.vertices[0], face.vertices[1], face.vertices[2]) for face in mesh.loop_triangles]
        
        f.write(struct.pack('i', len(mesh.loop_triangles)))
        
        flat_faces = []
        face_normals = []
        uv = []
        for face in mesh.loop_triangles:
            f.write(struct.pack('iii', *face.vertices))
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
        
        if len(mesh.materials) > 0:
            f.write(struct.pack('i', 1)) #material flag
            material = mesh.materials[0] #only first material
            col = material.diffuse_color
            if material.node_tree:
                for node in material.node_tree.nodes:
                    if node.bl_static_type == "BSDF_PRINCIPLED":
                        col = node.inputs[0].default_value
          
            f.write(struct.pack('ffff', *col))
            has_texture = False
            if material.node_tree:
                for node in material.node_tree.nodes:
                    if node.bl_static_type == "TEX_IMAGE":
                        texture = Path(node.image.filepath).name
                        f.write(struct.pack('i', len(texture)))
                        f.write(bytes(texture, "ascii"))
                        has_texture = True
                        #print(texture)
                        break
            if not has_texture:
                f.write(struct.pack('i', 0)) #no texture flag
        else:
            f.write(struct.pack('i', 0)) #no material flag
        
        armature = obj.find_armature()
                
        if armature:
            pose = armature.pose
            world = armature.matrix_world
            #local = armature.matrix_local
            #armature = armature.data
            
            #TODO: armature object transform?
            
            f.write(struct.pack('i', len(pose.bones)))
               
            bones_map = {bone.name: i for i, bone in enumerate(pose.bones)}
            for vert in mesh.vertices:
                f.write(struct.pack('i', len(vert.groups)))
                for wgrp in vert.groups:
                    group = obj.vertex_groups[wgrp.group]
                    bone_idx = bones_map[group.name]
                    f.write(struct.pack('i', bone_idx))
                    f.write(struct.pack('f', wgrp.weight))
                    
            if export_anim_setting:
                f.write(struct.pack('i', context.scene.frame_end))
                for frame in range(context.scene.frame_end):
                    context.scene.frame_set(frame)
                    write_frame_data(pose, obj.matrix_local, f)
                    print(frame)

            else:
                f.write(struct.pack('i', 1)) #single frame flag
                write_frame_data(pose, obj.matrix_local, f)
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
        default=True,
    )


    def execute(self, context):
        return write_some_data(context, self.filepath, self.export_anim)


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