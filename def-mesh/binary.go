components {
  id: "binary"
  component: "/def-mesh/binary.script"
}
embedded_components {
  id: "factory"
  type: "factory"
  data: "prototype: \"/def-mesh/dummy.go\"\n"
  ""
}
embedded_components {
  id: "dummy"
  type: "model"
  data: "mesh: \"/builtins/assets/meshes/sphere.dae\"\n"
  "name: \"{{NAME}}\"\n"
  "materials {\n"
  "  name: \"default\"\n"
  "  material: \"/builtins/materials/model.material\"\n"
  "  textures {\n"
  "    sampler: \"tex0\"\n"
  "    texture: \"/def-mesh/checker_256_32.png\"\n"
  "  }\n"
  "}\n"
  ""
}
embedded_components {
  id: "factory_bone"
  type: "factory"
  data: "prototype: \"/def-mesh/bone.go\"\n"
  ""
}
embedded_components {
  id: "factory_trans"
  type: "factory"
  data: "prototype: \"/def-mesh/dummy_trans.go\"\n"
  ""
}
