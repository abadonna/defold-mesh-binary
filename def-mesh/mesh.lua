local SETTINGS = require "def-mesh.settings"
local M = {}

--local D = require "def-mesh.dquats"

local vec3 = vmath.vector3

local function quat_to_euler (q) 
	local sinr_cosp = 2 * (q.w * q.x + q.y * q.z)
	local cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y)
	local x = math.atan2(sinr_cosp, cosr_cosp)

	local sinp = math.sqrt(1 + 2 * (q.w * q.y - q.x * q.z))
	local cosp = math.sqrt(1 - 2 * (q.w * q.y - q.x * q.z))
	local y = 2 * math.atan2(sinp, cosp) - math.pi / 2

	local siny_cosp = 2 * (q.w * q.z + q.x * q.y)
	local cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z)
	local z = math.atan2(siny_cosp, cosy_cosp)

	return math.deg(x), math.deg(y), math.deg(z)
end

local function mat_to_quat(m)
	local t = 0
	local q = nil
	if m.m22 < 0 then
		if m.m00 > m.m11 then
			t = 1 + m.m00 -m.m11 -m.m22
			q = vmath.quat(t, m.m01 + m.m10, m.m20 + m.m02, m.m21 - m.m12)
		else
			t = 1 - m.m00 + m.m11 - m.m22
			q = vmath.quat(m.m01 + m.m10, t, m.m12 + m.m21, m.m02 - m.m20)
		end

	else 
		if m.m00 < -m.m11 then 
			t = 1 - m.m00 - m.m11 + m.m22
			q = vmath.quat(m.m20 + m.m02, m.m12 + m.m21, t, m.m10 - m.m01)
		else 
			t = 1 + m.m00 + m.m11 + m.m22;
			q = vmath.quat(m.m21-m.m12, m.m02-m.m20, m.m10-m.m01, t)
		end
	end

	local st = math.sqrt(t)
	q.x = q.x * 0.5 / st
	q.y = q.y * 0.5 / st
	q.z = q.z * 0.5 / st
	q.w = q.w * 0.5 / st
	return q
end

M.new = function()
	local mesh = {}
	mesh.color = vmath.vector4(1.0, 1.0, 1.0, 1.0)
	mesh.bones_go = {}

	mesh.set_shapes = function(input)
		for name, value in pairs(input) do
			if mesh.shapes[name] then
				mesh.shapes[name].value = value
			end
		end
		if mesh.buf then
			mesh.update_vertex_buffer(input)
		end
	end

	mesh.interpolate = function(idx1, idx2, factor)
		
		if mesh.cache.idx2 == idx2 and mesh.cache.factor == factor then
			return mesh.cache.bones
		end
		local src = mesh.cache.bones or mesh.frames[idx2]
		local bones = {}
		
		for i = 1, #mesh.frames[idx1], 3 do 
			
			local m1 = vmath.matrix4()
			m1.c0 = mesh.frames[idx1][i]
			m1.c1 = mesh.frames[idx1][i + 1]
			m1.c2 = mesh.frames[idx1][i + 2]

			local m2 = vmath.matrix4();
			m2.c0 = src[i]
			m2.c1 = src[i + 1]
			m2.c2 = src[i + 2]

			local m = vmath.matrix4()

			-- dual quats?
			-- https://github.com/PacktPublishing/OpenGL-Build-High-Performance-Graphics/blob/master/Module%201/Chapter08/DualQuaternionSkinning/main.cpp
			-- https://subscription.packtpub.com/book/application_development/9781788296724/1/ch01lvl1sec09/8-skeletal-and-physically-based-simulation-on-the-gpu
			-- https://xbdev.net/misc_demos/demos/dual_quaternions_beyond/paper.pdf
			-- https://github.com/chinedufn/skeletal-animation-system/blob/master/src/blend-dual-quaternions.js
			-- https://github.com/Achllle/dual_quaternions/blob/master/src/dual_quaternions/dual_quaternions.py

			local t1 = vec3(m1.m30, m1.m31, m1.m32)
			local t2 = vec3(m2.m30, m2.m31, m2.m32)
			local t = vmath.lerp(factor, t1, t2)

			if mesh.inv_local_bones then
				local q1 = mat_to_quat(m1)
				local q2 = mat_to_quat(m2)
				local q =  vmath.slerp(factor, q1, q2)
				m = vmath.matrix4_from_quat(q)
			else --precomputed, blending will be incorrect anyway
				m.c0 = vmath.lerp(factor, m1.c0, m2.c0)
				m.c1 = vmath.lerp(factor, m1.c1, m2.c1)
				m.c2 = vmath.lerp(factor, m1.c2, m2.c2)
			end
		
			m.m30 = t.x
			m.m31 = t.y
			m.m32 = t.z
			m.m33 = 1.
			
			--m = D.FromMatrix(D.DualQuatToMatrix(D.DualQuatLerp(D.DualQuatFromMatrix(D.Matrix(m1)), D.DualQuatFromMatrix(D.Matrix(m2)), factor)))
			
			bones[i] = m.c0
			bones[i + 1] = m.c1
			bones[i + 2] = m.c2
		end
		
		return bones
	end
	
	mesh.calc_tangents = function()
		mesh.tangents = {}
		mesh.bitangents = {}
		if not mesh.material.normal then
			return
		end
		
		for i = 1, #mesh.faces do
			local face = mesh.faces[i]
			local v = {mesh.vertices[face.v[1]], mesh.vertices[face.v[2]], mesh.vertices[face.v[3]]}
			
			local uv1 = vec3(mesh.texcords[(i-1) * 6 + 1], mesh.texcords[(i-1) * 6 + 2], 0)
			local uv2 = vec3(mesh.texcords[(i-1) * 6 + 3], mesh.texcords[(i-1) * 6 + 4], 0)
			local uv3 = vec3(mesh.texcords[(i-1) * 6 + 5], mesh.texcords[(i-1) * 6 + 6], 0)

			local delta_pos1 = v[2].p - v[1].p;
			local delta_pos2 = v[3].p - v[1].p;

			local delta_uv1 = uv2 - uv1;
			local delta_uv2 = uv3 - uv1;

			local r = 1.0 / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);
			local tangent = (delta_pos1 * delta_uv2.y - delta_pos2 * delta_uv1.y) * r;
			local bitangent = (delta_pos2 * delta_uv1.x - delta_pos1 * delta_uv2.x) * r;

			--- move bitangent computation to vertex shader? E.g. pass bitangent sign as tangent.w ans use cross(T,N)
			for j = 1, 3 do
				if math.abs(r) < math.huge and vmath.dot(vmath.cross(v[j].n, tangent), bitangent) < 0.0 then
					table.insert(mesh.tangents, -tangent.x)
					table.insert(mesh.tangents, -tangent.y)
					table.insert(mesh.tangents, -tangent.z)
				else
					table.insert(mesh.tangents, tangent.x)
					table.insert(mesh.tangents, tangent.y)
					table.insert(mesh.tangents, tangent.z)
				end
				
				table.insert(mesh.bitangents, bitangent.x)
				table.insert(mesh.bitangents, bitangent.y)
				table.insert(mesh.bitangents, bitangent.z)
			end
		end
	end

	mesh.calculate_bones = function()
		if not mesh.cache.bones then
			return
		end

		if not mesh.inv_local_bones then --precomputed
			mesh.cache.calculated = mesh.cache.bones
			return
		end

		local inv_local = vmath.inv(mesh.local_.matrix)
		local bones = {}
		for idx = 1, #mesh.cache.bones, 3 do
			local bone = vmath.matrix4()
			bone.c0 = mesh.cache.bones[idx]
			bone.c1 = mesh.cache.bones[idx + 1]
			bone.c2 = mesh.cache.bones[idx + 2]

			local bone_local = vmath.matrix4()
			bone_local.c0 = mesh.inv_local_bones[idx]
			bone_local.c1 = mesh.inv_local_bones[idx + 1]
			bone_local.c2 = mesh.inv_local_bones[idx + 2]

			bone = mesh.local_.matrix * bone_local * bone * inv_local
			table.insert(bones, bone.c0)
			table.insert(bones, bone.c1)
			table.insert(bones, bone.c2)
		end

		mesh.cache.calculated = bones
	end

	mesh.apply_armature = function()
		if not mesh.cache.bones then
			return
		end

		if not mesh.cache.calculated then
			mesh.calculate_bones()
		end

		for idx, _ in pairs(mesh.used_bones_idx) do -- set only used bones, critical for performance
			local offset = idx * 3;
			
			for i = 1, 3 do
				go.set(mesh.url, "bones", mesh.cache.calculated[offset + i], {index = offset + i})
			end
		end
		
	end

	mesh.apply_transform = function(bone_id, url)
		
		local offset = (bone_id - 1) * 3
		
		local v1 = mesh.cache.calculated[offset + 1]
		local v2 = mesh.cache.calculated[offset + 2]
		local v3 = mesh.cache.calculated[offset + 3]

		local m = vmath.matrix4()
		m.c0 = vmath.vector4(v1.x, v2.x, v3.x, 1)
		m.c1 = vmath.vector4(v1.y, v2.y, v3.y, 1)
		m.c2 = vmath.vector4(v1.z, v2.z, v3.z, 1)
		m.c3 = vmath.vector4(v1.w, v2.w, v3.w, 1)

		local x,y,z = quat_to_euler(mat_to_quat(m))

		go.set(url, "euler.x", x)
		go.set(url, "euler.y", z)
		go.set(url, "euler.z", -y)

		local p = vmath.vector3(v1.w, v3.w, -v2.w);
		go.set_position(p, url)
	end

	mesh.add_bone_go = function(url, bone_id)
		table.insert(mesh.bones_go, {bone_id = bone_id, url = url})
		mesh.apply_transform(bone_id, url)
	end

	mesh.set_frame = function(idx1, idx2, factor)	
		if mesh.frames then
			idx = math.min(idx1, #mesh.frames)

			if SETTINGS.animate_blendshapes and #mesh.shape_frames >= idx then
				local input = {}
				local need_update = false
				for name, value in pairs(mesh.shape_frames[idx]) do
					if math.abs(mesh.shape_values[name] - value) >= SETTINGS.blendshape_treshold then
						input[name] = value
						need_update = true
						mesh.shape_values[name] = value
					end
				end
				
				if mesh.buf and need_update then
					mesh.update_vertex_buffer(input)
				end
			end

			if mesh.animate_with_texture then -- TODO: frames interpolation
				go.set(mesh.url, "animation", vmath.vector4(1./mesh.animate_tex_width, (idx - 1.)/mesh.animate_tex_height, 0., 0.))
			end

			if not mesh.animate_with_texture or #mesh.bones_go > 0 then
				idx2 = idx2 and math.min(idx2, #mesh.frames) or nil
				if mesh.cache.idx1 ~= idx1 or mesh.cache.idx2 ~= idx2 or mesh.cache.factor ~= factor  then
					mesh.cache.bones = idx2 and mesh.interpolate(idx, idx2, factor) or mesh.frames[idx]

					if mesh.animate_with_texture then --TODO: frames interpolation
						mesh.cache.bones = mesh.frames[idx]
					end
					
					mesh.cache.idx1 = idx1
					mesh.cache.idx2 = idx2
					mesh.cache.factor = factor
					mesh.calculate_bones()
				end

				if not mesh.animate_with_texture then
					mesh.apply_armature()
				end
			end

			for _, data in ipairs(mesh.bones_go) do
				mesh.apply_transform(data.bone_id, data.url)
			end
		end
	end

	mesh.update_vertex_buffer = function(shapes) 
		local key = ""
		for name, shape in pairs(mesh.shapes) do
			key = key .. name .. (shape.value or mesh.shape_values[name])
		end

		local blended = mesh.cache.blended;
		local need_update = false
		local need_update_tangets = SETTINGS.animate_tangents and (mesh.material.normal and true or false)

		if mesh.cache.shape_key == key then
			need_update = mesh_utils.apply_shapes(mesh.buf, mesh, mesh.shape_values, shapes, blended, need_update_tangets)
		else

			mesh_utils.clear_cache(blended)
			mesh.cache.blended = nil
			need_update, blended = mesh_utils.apply_shapes(mesh.buf, mesh, mesh.shape_values, shapes, nil, need_update_tangets)

			if blended then
				mesh.cache.blended = blended
				mesh.cache.shape_key = key
			end
		end

		if need_update then
			resource.set_buffer(go.get(mesh.url, "vertices"), mesh.buf)
		end 
		
	end
	
	mesh.create_buffer = function()
		local buf = buffer.create(#mesh.faces * 3, {
			{name = hash("position"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("normal"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("texcoord0"), type = buffer.VALUE_TYPE_FLOAT32, count = 2},
			{name = hash("weight"), type = buffer.VALUE_TYPE_FLOAT32, count = 4},
			{name = hash("bone"), type = buffer.VALUE_TYPE_UINT8, count = 4},
			{name = hash("tangent"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("bitangent"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
		})
	
		local weights = buffer.get_stream(buf, "weight")
		local bones = buffer.get_stream(buf, "bone")

		local positions = buffer.get_stream(buf, "position")
		local normals = buffer.get_stream(buf, "normal")

		local tangents = buffer.get_stream(buf, "tangent")
		local bitangents = buffer.get_stream(buf, "bitangent")

		mesh.calc_tangents()
		
		local blended = {}
		mesh.indices = {}
		
		if mesh.cache.blended_all then
			blended = mesh.cache.blended_all
		else
			--apply blend shapes 
			for idx, vertex in ipairs(mesh.vertices) do
				local v = { p = vec3(), n = vec3(), q = vmath.quat() }
				
				local total_weight = 0
				local iq = vmath.quat()
				for shape_name, shape in pairs(mesh.shapes) do
					local delta = shape.deltas[idx]
					local value = shape.value or mesh.shape_values[shape_name]
					if delta then
						total_weight = total_weight + value
						v.p = v.p + value * delta.p
						v.n = v.n + value * delta.n
						v.q = v.q * vmath.lerp(value, iq, delta.q)
					end
				end

				if total_weight > 1 then
					v.p = vertex.p + v.p / total_weight
					v.n = vertex.n + v.n / total_weight
				elseif total_weight > 0 then
					v.p = vertex.p + v.p
					v.n = vertex.n + v.n
				else
					v = vertex
				end

				blended[idx]  = v
			end
			mesh.cache.blended_all = blended
		end

		local count = 1
		local bcount = 1

		mesh.used_bones_idx = {}
		for i, face in ipairs(mesh.faces) do
			for _, idx in ipairs(face.v) do

				if not mesh.indices[idx] then
					mesh.indices[idx] = {}
				end
				table.insert(mesh.indices[idx], count)
				
				local vertex = blended[idx]
				local n = face.n or vertex.n

				positions[count] = vertex.p.x
				positions[count + 1] = vertex.p.y
				positions[count + 2] = vertex.p.z
				
				normals[count] = n.x
				normals[count + 1] = n.y
				normals[count + 2] = n.z
				--TOFIX: not correct for blendshaped face normals

				if mesh.material.normal then
					if not vertex.q then
						tangents[count] = mesh.tangents[count]
						tangents[count + 1] = mesh.tangents[count + 1]
						tangents[count + 2] = mesh.tangents[count + 2]

						bitangents[count] = mesh.bitangents[count]
						bitangents[count + 1] = mesh.bitangents[count + 1]
						bitangents[count + 2] = mesh.bitangents[count + 2]
					else
						local t = vmath.rotate(vertex.q, vec3(mesh.tangents[count], mesh.tangents[count + 1], mesh.tangents[count + 2]))
						tangents[count] = t.x
						tangents[count + 1] = t.y
						tangents[count + 2] = t.z

						local bt = vmath.rotate(vertex.q, vec3(mesh.bitangents[count], mesh.bitangents[count + 1], mesh.bitangents[count + 2]))
						bitangents[count] = bt.x
						bitangents[count + 1] = bt.y
						bitangents[count + 2] = bt.z
					end
				end
				
				count = count + 3

				local skin = mesh.skin and mesh.skin[idx] or nil
				local bone_count = skin and #skin or 0

				for j = 1, bone_count do
					mesh.used_bones_idx[skin[j].idx] = true
				end
				
				weights[bcount] = bone_count > 0 and skin[1].weight or 0
				weights[bcount + 1] = bone_count > 1 and skin[2].weight or 0
				weights[bcount + 2] = bone_count > 2 and skin[3].weight or 0
				weights[bcount + 3] = bone_count > 3 and skin[4].weight or 0

				bones[bcount] = bone_count > 0 and skin[1].idx or 0
				bones[bcount + 1] = bone_count > 1 and skin[2].idx or 0
				bones[bcount + 2] = bone_count > 2 and skin[3].idx or 0
				bones[bcount + 3] = bone_count > 3 and skin[4].idx or 0

				bcount = bcount + 4
			end
		end

		local texcords = buffer.get_stream(buf, "texcoord0")
		for i, value in ipairs(mesh.texcords) do
			texcords[i] = value
		end

		if mesh.shapes ~= {} then
			if mesh.cache.vertex_data == nil then
				mesh.cache.vertex_data = mesh_utils.load_vertex_data(mesh)
			end
			mesh.data = mesh_utils.load_mesh_data(mesh)
			mesh.buf = buf
		end

		return buf
	end
	
	return mesh
end 

return M