local Mesh = require "def-mesh.mesh"


local function prepare_submeshes(meshes)
	local mesh = meshes[1]
	mesh.calc_tangents()
	
	if #meshes < 2 then
		return meshes
	end

	for i = #meshes, 2, -1 do
		local m = meshes[i]
		if #m.faces == 0 then
			table.remove(meshes, i)
		else
			m.local_ = mesh.local_
			m.world_ = mesh.world_
			m.vertices = mesh.vertices
			m.position = mesh.position
			m.rotation = mesh.rotation
			m.scale = mesh.scale
			m.skin = mesh.skin
			m.frames = mesh.frames
			m.bones = mesh.bones
			m.calc_tangents()
		end
	end
	return meshes
end

local M = {}

M.init_from_resource = function(path)
	M.content = sys.load_resource(path)
	M.index = 1
end

M.read_mesh = function()
	local mesh = Mesh.new()
	local meshes = {mesh}
	
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

	local face_map = {}
	local face_count = M.read_int()
	mesh.faces = {}
	for i = 1, face_count do
		local face = {
			v = {
				M.read_int() + 1, 
				M.read_int() + 1, 
				M.read_int() + 1
			}
		}
		local mi = M.read_int() + 1
		table.insert(face_map, mi)

		local flat_flag = M.read_int()
		if flat_flag == 1 then
			face.n = M.read_vec3()
		end
		
		while #meshes < mi do
			local m = Mesh.new()
			m.name = mesh.name .. "_" .. #meshes
			m.faces = {}
			m.texcords = {}
			m.tangents = {}
			m.bitangents = {}
			table.insert(meshes, m)
		end
		table.insert(meshes[mi].faces, face)
	end

	mesh.texcords = {}
	for i = 1, face_count do
		local m = meshes[face_map[i]]
		for j = 1, 6 do
			table.insert(m.texcords, M.read_float())
		end
	end
	
	local material_count = M.read_int()
	
	for i = 1, material_count do
		local m = meshes[i] or {} --not used material
		
		m.material = {
			type = M.read_int(), -- 0 - opaque, 1 - transparent
			color = M.read_vec4(), 
			specular =  M.read_float(),
			roughness =  M.read_float()}
		local texture_flag = M.read_int()
		if texture_flag > 0 then
			m.material.texture = M.read_string(texture_flag)
		end
		texture_flag = M.read_int() --normal texture
		if texture_flag > 0 then
			m.material.texture_normal = M.read_string(texture_flag)
			m.material.normal_strength = M.read_float()
		end
		texture_flag = M.read_int() --specular texture
		if texture_flag > 0 then
			m.material.texture_specular = M.read_string(texture_flag)
			m.material.specular_invert = M.read_int()
		end
		texture_flag = M.read_int() --roughness texture
		if texture_flag > 0 then
			m.material.texture_roughness = M.read_string(texture_flag)
		end
	end

	local bone_count = M.read_int()
	
	if bone_count == 0 then
		mesh.position = mesh.local_.position
		mesh.rotation = mesh.local_.rotation
		mesh.scale = mesh.local_.scale
		return copy_data(meshes)
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

	return prepare_submeshes(meshes)
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