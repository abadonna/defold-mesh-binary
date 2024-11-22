import bpy


def export_animations(context, filepath):
    obj = context.active_object
    if obj.type != 'ARMATURE':
        print("Not an armature")
        return

    file = open(filepath, 'w')
    file.write('return {\n')
    
    animation_data = {}
    scene = bpy.context.scene

    if obj.animation_data and obj.animation_data.nla_tracks:
        for track in obj.animation_data.nla_tracks:
            for strip in track.strips:
                file.write('\t' + strip.name + ' = {start = ' + str(strip.frame_start) + ', finish = ' + str(strip.frame_end) + '},\n')

    file.write('}')
    file.close()
    return {'FINISHED'}

# ExportHelper is a helper class, defines filename and
# invoke() function which calls the file selector.
from bpy_extras.io_utils import ExportHelper
from bpy.types import Operator

class NLAExport(Operator, ExportHelper):
    """This appears in the tooltip of the operator and in the generated docs"""
    bl_idname = "export_nla.defold"  # important since its how bpy.ops.import_test.some_data is constructed
    bl_label = "Export nla"
    bl_description  = "Export nla tracks data"

    # ExportHelper mixin class uses this
    filename_ext = ".lua"

    def execute(self, context):
        return export_animations(context, self.filepath)


bpy.utils.register_class(NLAExport)
bpy.ops.export_nla.defold('INVOKE_DEFAULT')
