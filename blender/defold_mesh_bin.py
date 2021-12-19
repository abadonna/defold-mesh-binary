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


def write_some_data(context, filepath):
    
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
                        
        f.write(struct.pack('fff',obj.location[0], obj.location[2], -obj.location[1]))
        f.write(struct.pack('fff',obj.rotation_euler[0], obj.rotation_euler[2], -obj.rotation_euler[1]))
        f.write(struct.pack('fff',obj.scale[0], obj.scale[2], obj.scale[1]))
                 
        mesh.calc_loop_triangles()
        mesh.calc_normals_split()
        
        f.write(struct.pack('i', len(mesh.vertices)))
        
        for vert in mesh.vertices:
            f.write(struct.pack('fff',vert.co.x, vert.co.z, -vert.co.y))
        
        for vert in mesh.vertices:
            f.write(struct.pack('fff',vert.normal.x, vert.normal.z, -vert.normal.y,))
       
        #vertices = [(vert.co.x, vert.co.y, vert.co.z) for vert in mesh.vertices]
        #normals = [(vert.normal.x, vert.normal.y, vert.normal.z) for vert in mesh.vertices]
        #faces = [(face.vertices[0], face.vertices[1], face.vertices[2]) for face in mesh.loop_triangles]
        
        f.write(struct.pack('i', len(mesh.loop_triangles)))
        
        flat_faces = []
        face_normals = []
        uv = []
        for face in mesh.loop_triangles:
            f.write(struct.pack('iii',face.vertices[0], face.vertices[1], face.vertices[2]))
            for loop_idx in face.loops:
                uv_cords = mesh.uv_layers.active.data[loop_idx].uv if mesh.uv_layers.active else (0, 0)
                uv.extend((uv_cords[0], uv_cords[1]))
            if not face.use_smooth:
                flat_faces.append(face.index)
                face_normals.extend((face.normal.x, face.normal.z, -face.normal.y))
        
        #f.write(struct.pack('i', len(uv)))
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
          
            f.write(struct.pack('ffff',col[0],col[1],col[2],col[3]))
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
        
        
    #f.write("Hello World")
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


    def execute(self, context):
        return write_some_data(context, self.filepath)


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