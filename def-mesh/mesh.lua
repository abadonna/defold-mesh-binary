local M = {}

M.new = function()
	local mesh = {}
	mesh.color = vmath.vector4(1.0, 1.0, 1.0, 1.0)
	mesh.calc_tangents = function()
		mesh.tangents = {}
		mesh.bitangents = {}
		if not mesh.material.texture_normal then
			return
		end

		for i = 1, #mesh.faces do
			local face = mesh.faces[i]
			local v = {mesh.vertices[face.v[1]], mesh.vertices[face.v[2]], mesh.vertices[face.v[3]]}
			
			local uv1 = vmath.vector3(mesh.texcords[(i-1) * 6 + 1], mesh.texcords[(i-1) * 6 + 2], 0)
			local uv2 = vmath.vector3(mesh.texcords[(i-1) * 6 + 3], mesh.texcords[(i-1) * 6 + 4], 0)
			local uv3 = vmath.vector3(mesh.texcords[(i-1) * 6 + 5], mesh.texcords[(i-1) * 6 + 6], 0)

			local delta_pos1 = v[2].p - v[1].p;
			local delta_pos2 = v[3].p - v[1].p;

			local delta_uv1 = uv2 - uv1;
			local delta_uv2 = uv3 - uv1;

			local r = 1.0 / (delta_uv1.x * delta_uv2.y - delta_uv1.y * delta_uv2.x);
			local tangent = (delta_pos1 * delta_uv2.y - delta_pos2 * delta_uv1.y) * r;
			local bitangent = (delta_pos2 * delta_uv1.x - delta_pos1 * delta_uv2.x) * r;

			for j = 1, 3 do
				if r ~= math.huge and vmath.dot(vmath.cross(v[j].n, tangent), bitangent) < 0.0 then
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

	mesh.apply_armature = function()
		if not mesh.bones then
			return
		end

		--[[
		for i, v in ipairs(mesh.bones) do
			go.set(mesh.url, "bones", v, {index = i})
		end --]]

		for idx, _ in pairs(mesh.used_bones_idx) do -- set only used bones, critical for performance
			local offset = idx * 4;
			for i = 1, 4 do
				go.set(mesh.url, "bones", mesh.bones[offset + i], {index = offset + i})
			end
		end
		
	end

	mesh.set_frame = function(idx, url)
		if mesh.frames then
			mesh.bones = mesh.frames[idx]
			mesh.apply_armature(url)
		end
	end
	
	mesh.create_buffer = function()
		local buf = buffer.create(#mesh.faces * 3, {
			{name = hash("position"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("normal"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("texcoord0"), type = buffer.VALUE_TYPE_FLOAT32, count = 2},
			{name = hash("weight"), type = buffer.VALUE_TYPE_FLOAT32, count = 4},
			{name = hash("bone"), type = buffer.VALUE_TYPE_UINT8, count = 4},
			{name = hash("color"), type = buffer.VALUE_TYPE_FLOAT32, count = 4},
			{name = hash("tangent"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("bitangent"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
		})
		local positions = buffer.get_stream(buf, "position")
		local normals = buffer.get_stream(buf, "normal")
		local weights = buffer.get_stream(buf, "weight")
		local bones = buffer.get_stream(buf, "bone")
		local color = buffer.get_stream(buf, "color")

		local count = 1
		local bcount = 1

		mesh.used_bones_idx = {}
		for i, face in ipairs(mesh.faces) do
			for _, idx in ipairs(face.v) do
				local vertex = mesh.vertices[idx]

				positions[count] = vertex.p.x
				positions[count + 1] = vertex.p.y
				positions[count + 2] = vertex.p.z

				local n = face.n or vertex.n
				normals[count] = n.x
				normals[count + 1] = n.y
				normals[count + 2] = n.z
				
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

				color[bcount] = mesh.material and mesh.material.color.x or 0.8
				color[bcount + 1] = mesh.material and mesh.material.color.y or 0.8
				color[bcount + 2] = mesh.material and mesh.material.color.z or 0.8
				color[bcount + 3] = mesh.material and mesh.material.color.w or 1.0

				bcount = bcount + 4
			end
		end

		local texcords = buffer.get_stream(buf, "texcoord0")
		for i, value in ipairs(mesh.texcords) do
			texcords[i] = value
		end

		local tangents = buffer.get_stream(buf, "tangent")
		for i, value in ipairs(mesh.tangents) do
			tangents[i] = value
		end

		local bitangents = buffer.get_stream(buf, "bitangent")
		for i, value in ipairs(mesh.bitangents) do
			bitangents[i] = value
		end

		return buf
	end
	
	return mesh
end 

return M