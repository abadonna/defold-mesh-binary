# defold-mesh-binary
proof of concept

## Usage
1. Use defold_mesh_bin.py to export scene from Blender (tested only in 3.0.0)
2. Add def-mesh/binary.go into the scene and set all textures you need
3. Send "load_mesh" message with path to binary file in custom resources

## Features
* Binary format
* Exports all the meshes
* Base color and texture for every material (so far only "Principled BSDF")
* Multiple materials per mesh, up to 8 textures per mesh
* Bones
* Bone animations on GPU

## Drawbacks (Defold 190)
* Impossible to create vertex buffer in runtime, so we have to keep buffers pool
* Impossible to create texture in runtime
* Meshes are not visible in Editor, only in runtime

## TODO
* More advanced materials, search deeper links for textures
---