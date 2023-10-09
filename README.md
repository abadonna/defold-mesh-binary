# defold-mesh-binary
Export mesh of any complexity from Blender to Defold game engine.

[demo on github pages](https://abadonna.github.io/defold-mesh-binary/)

## Usage
1. Use defold_mesh_bin.py to export scene from Blender (tested only in 3.0.0)
2. Add def-mesh/binary.go into the scene and set all textures you need
3. Send "load_mesh" message with path to binary file in custom resources and path to texture folder
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
* Exports all the meshes
* Base color, specular power, roughness and texture for every material (so far only "Principled BSDF")
* Normal map, reflection map, roughness map
* Blend shapes
* Multiple materials per mesh
* Bones
* Bone animations on GPU
* Blend shape animations in C
* Transparent materials

## Experimental
* animations baked into texture
* GameObject can be attached to bone

## Drawbacks
* Meshes are not visible in Editor, only in runtime
There is a dummy model in binary.go, so any mesh can be set on it to make scene more obvious in editor. Dummy model is disabled in runtime. 


## TODO
* Blending baked animations 
* "Import animation only" option for exporter

---