local M = {}

M.new = function()
	local mesh = {}
	mesh.color = vmath.vector4(1.0, 1.0, 1.0, 1.0)

	mesh.apply_armature = function(url)
		if not mesh.bones then
			return
		end

		for i, v in ipairs(mesh.bones) do
			go.set(url, "bones", v, {index = i})
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
			{name = hash("material"), type = buffer.VALUE_TYPE_FLOAT32, count = 4},
			{name = hash("color"), type = buffer.VALUE_TYPE_FLOAT32, count = 4},
		})
		local positions = buffer.get_stream(buf, "position")
		local normals = buffer.get_stream(buf, "normal")
		local weights = buffer.get_stream(buf, "weight")
		local bones = buffer.get_stream(buf, "bone")
		local material = buffer.get_stream(buf, "material")
		local color = buffer.get_stream(buf, "color")

		local count = 1
		local bcount = 1
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
				
				weights[bcount] = bone_count > 0 and skin[1].weight or 0
				weights[bcount + 1] = bone_count > 1 and skin[2].weight or 0
				weights[bcount + 2] = bone_count > 2 and skin[3].weight or 0
				weights[bcount + 3] = bone_count > 3 and skin[4].weight or 0

				bones[bcount] = bone_count > 0 and skin[1].idx or 0
				bones[bcount + 1] = bone_count > 1 and skin[2].idx or 0
				bones[bcount + 2] = bone_count > 2 and skin[3].idx or 0
				bones[bcount + 3] = bone_count > 3 and skin[4].idx or 0

				local m = mesh.materials[face.mi]
				
			
				color[bcount] = m and m.color.x or 0.8
				color[bcount + 1] = m and m.color.y or 0.8
				color[bcount + 2] = m and m.color.z or 0.8
				color[bcount + 3] = m and m.color.w or 1.0

				if m and m.tex_id then
					material[bcount] = m.tex_id
				else
					material[bcount] = 0
				end
				material[bcount + 1] = 0
				material[bcount + 2] = 0
				material[bcount + 3] = 0
				
					
				bcount = bcount + 4
			end
		end

		local texcords = buffer.get_stream(buf, "texcoord0")
		for i, value in ipairs(mesh.texcords) do
			texcords[i] = value
		end

		return buf
	end
	
	return mesh
end 

return M