local Mesh = require "def-mesh.mesh"

local function decompose(src)
	local trans_materials = {}
	local trans_material_map = {}

	local materials = {}
	local material_map = {}

	local trans_texture_indxs = {}
	local texture_indxs = {}
	
	for i, material in ipairs(src.materials) do
		if material.type > 0 then
			table.insert(trans_materials, material)
			trans_material_map[i] = #trans_materials
			if material.tex_id then
				trans_texture_indxs[material.tex_id] = true
			end
		else
			table.insert(materials, material)
			material_map[i] = #materials
			if material.tex_id then
				texture_indxs[material.tex_id] = true
			end
		end
	end

	if #trans_materials == 0 or #materials == 0 then
		return {src}
	end

	local mesh = Mesh.new()

	local tex_map = {}
	mesh.textures = {}

	for key in pairs(trans_texture_indxs) do
		table.insert(mesh.textures, src.textures[key])
		tex_map[key] = #mesh.textures
	end
	
	for _, material in ipairs(trans_materials) do
		if material.tex_id then
			material.tex_id = tex_map[material.tex_id]
		end
	end
	
	mesh.name = src.name .. "_trans"
	mesh.materials = trans_materials
	mesh.local_ = src.local_
	mesh.world_ = src.world_
	mesh.vertices = src.vertices
	mesh.faces = {}
	mesh.texcords = {}
	mesh.position = src.position
	mesh.rotation = src.rotation
	mesh.scale = src.scale
	mesh.skin = src.skin
	mesh.frames = src.frames
	mesh.bones = src.bones

	for i = #src.faces, 1, -1 do
		local face = src.faces[i]
		if trans_material_map[face.mi] then
			table.remove(src.faces, i)
			face.mi = trans_material_map[face.mi]
			table.insert(mesh.faces, face)
			for j = 1, 3 do
				table.insert(mesh.texcords, table.remove(src.texcords, (i-1) * 6 + 1))
				table.insert(mesh.texcords, table.remove(src.texcords, (i-1) * 6 + 1))
			end
		else
			face.mi = material_map[face.mi]
		end
	end


	tex_map = {}
	local textures = {}

	for key in pairs(texture_indxs) do
		table.insert(textures, src.textures[key])
		tex_map[key] = #textures
	end

	for _, material in ipairs(materials) do
		if material.tex_id then
			material.tex_id = tex_map[material.tex_id]
		end
	end

	src.materials = materials
	src.textures = textures
	
	return {src, mesh}
end


local M = {}

M.init_from_resource = function(path)
	M.content = sys.load_resource(path)
	M.index = 1
end

M.read_mesh = function()
	local mesh = Mesh.new()

	mesh.name = M.read_string()
	
	local parent_flag = M.read_int()
	if parent_flag > 0 then
		mesh.parent =  M.read_string(parent_flag)
	end

	mesh.local_ = M.read_transform()
	mesh.world_ = M.read_transform()
	
	local vertex_count = M.read_int()
	mesh.vertices = {}
	for i = 1, vertex_count do
		table.insert(mesh.vertices, 
		{ 
			p = M.read_vec3(),
			n = M.read_vec3()
		})
	end

	local face_count = M.read_int()
	mesh.faces = {}
	for i = 1, face_count do
		table.insert(mesh.faces, 
		{
			v = {
				M.read_int() + 1, 
				M.read_int() + 1, 
				M.read_int() + 1
			},
			mi = M.read_int() + 1
		})
	end

	mesh.texcords = {}
	for i = 1, face_count * 6 do
		table.insert(mesh.texcords, M.read_float())
	end

	local flat_face_count = M.read_int()
	local flat_faces = {}
	for i = 1, flat_face_count do
		table.insert(flat_faces, M.read_int() + 1)
	end

	for i = 1, flat_face_count do
		mesh.faces[flat_faces[i]].n = M.read_vec3()
	end

	
	local material_count = M.read_int()
	mesh.materials = {}
	mesh.textures = {}
	local tex_map = {}
	for i = 1, material_count do
		local material = {
			type = M.read_int(), -- 0 - opaque, 1 - transparent
			color = M.read_vec4(), 
			specular =  M.read_float()}
		local texture_flag = M.read_int()
		if texture_flag > 0 then
			local texture = M.read_string(texture_flag)
			if tex_map[texture] then
				material.tex_id = tex_map[texture]
			else
				table.insert(mesh.textures, texture)
				tex_map[texture] = #mesh.textures
				material.tex_id = tex_map[texture]
			end
		end
		table.insert(mesh.materials, material)
	end

	local bone_count = M.read_int()
	
	if bone_count == 0 then
		mesh.position = mesh.local_.position
		mesh.rotation = mesh.local_.rotation
		mesh.scale = mesh.local_.scale
		return mesh
	end

	mesh.position = mesh.world_.position
	mesh.rotation = mesh.world_.rotation
	mesh.scale = mesh.world_.scale

	--reading armature

	local sort_f = function(a,b) return a.weight > b.weight end

	mesh.skin = {}
	for i = 1, vertex_count do
		data = {}
		local weight_count = M.read_int()
		
		for j = 1, weight_count do
			table.insert(data, 
			{
				idx = M.read_int(),
				weight = M.read_float()
			})
		end

		table.insert(mesh.skin, data)
	end

	local frame_count = M.read_int()
	mesh.frames = {}
	for i = 1, frame_count do
		local bones = {}
		for j = 1, bone_count * 4 do
			table.insert(bones, M.read_vec4()) -- read matrices per column as vectors for simplicity
		end
		table.insert(mesh.frames, bones)
	end
	
	mesh.bones = mesh.frames[1]

	return decompose(mesh)
end

M.eof = function()
	return M.index > #M.content
end

M.read_string = function(size)
	local res = ""
	size = size or M.read_int()
	for i = 1, size do
		local b = string.byte(M.content, M.index)
		res = res .. string.char(b)
		M.index = M.index + 1
	end
	return res
end

M.read_int = function()
	local b4 = string.byte(M.content, M.index)
	local b3 = string.byte(M.content, M.index + 1)
	local b2 = string.byte(M.content, M.index + 2)
	local b1 = string.byte(M.content, M.index + 3)

	local n = b1 * 16777216 + b2 * 65536 + b3 * 256 + b4
	
	M.index = M.index + 4
	n = (n > 2147483647) and (n - 4294967296) or n
	return n
end

--http://lua-users.org/lists/lua-l/2010-03/msg00910.html
M.read_float = function()
	local sign = 1
	
	local mantissa = string.byte(M.content, M.index + 2) % 128
	for i = 1, 0, -1 do mantissa = mantissa * 256 + string.byte(M.content, M.index + i) end
	if string.byte(M.content, M.index + 3) > 127 then sign = -1 end
	local exponent = (string.byte(M.content, M.index + 3) % 128) * 2 + math.floor(string.byte(M.content, M.index + 2) / 128)
	M.index = M.index + 4
	if exponent == 0 then return 0 end
	mantissa = (math.ldexp(mantissa, -23) + 1) * sign
	return math.ldexp(mantissa, exponent - 127)
end

M.read_vec3 = function()
	return vmath.vector3(M.read_float(), M.read_float(), M.read_float())
end

M.read_vec4 = function()
	return vmath.vector4(M.read_float(), M.read_float(), M.read_float(), M.read_float())
end

M.read_quat = function()
	local v = M.read_vec4() 
	return vmath.quat(v.y, v.z, v.w, v.x) --Blender quat
end

M.read_matrix = function()
	local m = vmath.matrix4()
	m.c0 = M.read_vec4()
	m.c1 = M.read_vec4()
	m.c2 = M.read_vec4()
	m.c3 = M.read_vec4()
	return m
end

--read position, rotation and scale in Defold coordinates 
M.read_transform = function()
	local res = {}
	local v = M.read_vec3()
	res.position = vmath.vector3(v.x, v.z, -v.y)-- blender coords fix

	local euler = M.read_vec3()
	local qx = vmath.quat_rotation_x(euler.x)
	local qy = vmath.quat_rotation_y(euler.z) -- blender coords fix
	local qz = vmath.quat_rotation_z(-euler.y) -- blender coords fix
	res.rotation = qx * qz * qy

	v = M.read_vec3()
	res.scale = vmath.vector3(v.x, v.z, v.y)-- blender coords fix
	return res
end



return M