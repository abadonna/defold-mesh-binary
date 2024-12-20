# defold-mesh-binary
Export mesh of any complexity from Blender to Defold game engine.

**The code was rewritten with C++ for better performance, previous version is in** [lua-mesh branch](https://github.com/abadonna/defold-mesh-binary/tree/lua-mesh)

[demo on github pages](https://abadonna.github.io/defold-mesh-binary/)

## Usage
1. Use defold_mesh_bin.py to export scene from Blender (tested only in 3.0.0)
2. Add def-mesh/binary.go into the scene
3. Require module "def-mesh.binary" and call binary.load(url_to_binary_go, path_to_bin_file_in_resources)
4. For transparent materials, add "trans" predicate and render it after "model" predicate:
```` 
	render.set_depth_mask(false)
	render.disable_state(render.STATE_CULL_FACE)
	render.draw(self.trans_pred)
	render.set_depth_mask(true)
```` 

![pcss](https://github.com/abadonna/defold-mesh-binary/blob/main/sample.png)

## Features
* Binary format
* Half precision floats
* Exports all the meshes
* Base color, specular power, roughness and texture for every material (so far only "Principled BSDF")
* Normal map, reflection map, roughness map
* Blend shapes
* Multiple materials per mesh
* Bones
* Bone animations on GPU
* Blend shape animations
* Transparent materials
* Animation tracks
* Animations baked in textures (for performance)
* Editor script to generate animation masks
* Editor script to extract material list

## Experimental
* Root Motion (only on based track)
* GameObjects can be attached to bones

## Drawbacks
* Meshes are not visible in Editor, only in runtime
There is a dummy model in binary.go, so any mesh can be set on it to make scene more obvious in editor. Dummy model is disabled in runtime.

* It looks like animations (mostly Root Motion ones) should be (optionally) interpolated between frames to avoid jitter.


---