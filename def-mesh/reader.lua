local Mesh = require "def-mesh.mesh"
local function copy(obj, seen)
	if type(obj) ~= 'table' then return obj end
	if seen and seen[obj] then return seen[obj] end
	local s = seen or {}
	local res = setmetatable({}, getmetatable(obj))
	s[obj] = res
	for k, v in pairs(obj) do res[copy(k, s)] = copy(v, s) end
	return res
end

local function transpose(m)
	local r = vmath.matrix4(m)
	r.m01 = m.m10
	r.m02 = m.m20
	r.m03 = m.m30

	r.m10 = m.m01
	r.m12 = m.m21
	r.m13 = m.m31

	r.m20 = m.m02
	r.m21 = m.m12
	r.m23 = m.m32

	r.m30 = m.m03
	r.m31 = m.m13
	r.m32 = m.m23

	return r
end

local vec3 = vmath.vector3
local vec4 = vmath.vector4

local function prepare_submeshes(meshes)
	local mesh = meshes[1]
	
	if #meshes < 2 then
		return meshes
	end
	
	for i = #meshes, 1, -1 do
		local m = meshes[i]
		if #m.faces == 0 then
			table.remove(meshes, i)
		elseif i > 1 then
			m.parent = mesh.parent
			m.local_ = mesh.local_
			m.world_ = mesh.world_
			m.vertices = mesh.vertices
			m.position = mesh.position
			m.rotation = mesh.rotation
			m.scale = mesh.scale
			m.skin = mesh.skin
			m.frames = mesh.frames
			m.bones = mesh.bones
			m.inv_local_bones = mesh.inv_local_bones
			m.cache = mesh.cache
			m.shapes = mesh.shapes
			m.shape_frames = mesh.shape_frames
			m.shape_values = copy(mesh.shape_values)
		end
	end

	meshes[1].base = true
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
	mesh.base = true --not a submesh
	
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

	local shape_count = M.read_int()
	mesh.shapes = {}
	
	for j = 1, shape_count do
		local name = M.read_string()
		mesh.shapes[name] = {deltas = {}}
		local delta_count = M.read_int()
		
		for i = 1, delta_count do
			local idx = M.read_int() + 1
			mesh.shapes[name].deltas[idx] = 
			{ 
				p = M.read_vec3(),
				n = M.read_vec3()
			}
			mesh.shapes[name].deltas[idx].q = vmath.quat_from_to(mesh.vertices[idx].n, mesh.vertices[idx].n + mesh.shapes[name].deltas[idx].n)
		end
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
			type = M.read_int(), -- 0 - opaque, 1 - blend, 2 - hashed
			color = M.read_vec4(), 
			specular = {value = M.read_float()},
			roughness = {value = M.read_float()}
		}
		local texture_flag = M.read_int()
		local ramp_flag
		if texture_flag > 0 then
			m.material.texture = M.read_string(texture_flag)
		end
		texture_flag = M.read_int() --normal texture
		if texture_flag > 0 then
			m.material.normal = {
				texture = M.read_string(texture_flag),
				strength = M.read_float()
			}
		end
		texture_flag = M.read_int() --specular texture
		if texture_flag > 0 then
			m.material.specular.texture = M.read_string(texture_flag)
			m.material.specular.invert = M.read_int()
			ramp_flag = M.read_int()
			if ramp_flag > 0 then
				m.material.specular.ramp = {
					p1 = M.read_float(),
					v1 = M.read_float(),
					p2 = M.read_float(),
					v1 = M.read_float()
				}
			end
		end
		texture_flag = M.read_int() --roughness texture
		if texture_flag > 0 then
			m.material.roughness.texture = M.read_string(texture_flag)
			ramp_flag = M.read_int()
			if ramp_flag > 0 then
				m.material.roughness.ramp = {
					p1 = M.read_float(),
					v1 = M.read_float(),
					p2 = M.read_float(),
					v1 = M.read_float()
				}
			end
		end
	end

	local bone_count = M.read_int()
	
	if bone_count == 0 then
		mesh.position = mesh.local_.position
		mesh.rotation = mesh.local_.rotation
		mesh.scale = mesh.local_.scale
		mesh.cache = {}
		return prepare_submeshes(meshes)
	end

	mesh.position = mesh.world_.position
	mesh.rotation = mesh.world_.rotation
	mesh.scale = mesh.world_.scale

	--reading armature
	--TODO: keep rigs separate from meshes for optimization

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

	local precomputed = (M.read_int() == 1)

	if not precomputed then
		mesh.inv_local_bones = {}
		for i = 1, bone_count  do
			--3x4 transform matrix
			table.insert(mesh.inv_local_bones, M.read_vec4()) 
			table.insert(mesh.inv_local_bones, M.read_vec4()) 
			table.insert(mesh.inv_local_bones, M.read_vec4()) 
		end
	end

	local frame_count = M.read_int()
	mesh.frames = {}
	mesh.shape_frames = {}

	
	for i = 1, frame_count do
		local bones = {}
		for j = 1, bone_count  do
			--3x4 transform matrix
			table.insert(bones, M.read_vec4()) 
			table.insert(bones, M.read_vec4()) 
			table.insert(bones, M.read_vec4()) 
		end
		table.insert(mesh.frames, bones)

		local shapes = {}
		for j = 1, shape_count do
			shapes[M.read_string()] = M.read_float()
		end
		table.insert(mesh.shape_frames, shapes)
	end

	mesh.shape_values = copy(mesh.shape_frames[1])
	mesh.cache = {bones = mesh.frames[1]} -- to share data with submeshes

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
	return vec3(M.read_float(), M.read_float(), M.read_float())
end

M.read_vec4 = function()
	return vec4(M.read_float(), M.read_float(), M.read_float(), M.read_float())
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
	local p = M.read_vec3()
	res.position = vec3(p.x, p.z, -p.y)-- blender coords fix

	local euler = M.read_vec3()
	local qx = vmath.quat_rotation_x(euler.x)
	local qy = vmath.quat_rotation_y(euler.z) -- blender coords fix
	local qz = vmath.quat_rotation_z(-euler.y) -- blender coords fix
	res.rotation = qy * qz  * qx

	local s = M.read_vec3()
	res.scale = vec3(s.x, s.z, s.y)-- blender coords fix

	local mtx_rot = vmath.matrix4_rotation_x(euler.x) * vmath.matrix4_rotation_y(euler.y) * vmath.matrix4_rotation_z(euler.z) 
	local mtx_tr = vmath.matrix4_translation(p)
	local mtx_scale = vmath.matrix4()
	mtx_scale.m00 = s.x
	mtx_scale.m11 = s.y
	mtx_scale.m22 = s.z
	
	res.matrix =  transpose(mtx_tr * mtx_rot * mtx_scale)

	return res
end



return M