# defold-mesh-binary
proof of concept

## Usage
1. Use defold_mesh_bin.py to export scene from Blender (tested only in 3.0.0)
2. Add def-mesh/binary.go into the scene and set all textures you need
3. Send "load_mesh" message with path to binary file in custom resources
4. For transparent materials, add "trans" predicate and render it after "model" predicate:
```` 
	render.set_depth_mask(false)
	render.disable_state(render.STATE_CULL_FACE)
	render.draw(self.trans_pred)
	render.set_depth_mask(true)
```` 

## Features
* Binary format
* Exports all the meshes
* Base color, specular power, roughness and texture for every material (so far only "Principled BSDF")
* Normal map, reflection map, roughness map
* Multiple materials per mesh, up to 8 textures per mesh
* Bones
* Bone animations on GPU
* Transparent materials

## Drawbacks (Defold 190)
* Impossible to create vertex buffer in runtime, so we have to keep buffers pool
* Impossible to create texture in runtime
* Meshes are not visible in Editor, only in runtime

## ISSUES
[6281](https://github.com/defold/defold/issues/6281), [6283](https://github.com/defold/defold/issues/6283)

## TODO
* More advanced materials, with combined shader nodes
* Text mesh buffer for geometry, to make it visible in Editor (?)
---