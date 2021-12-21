local M = {}

M.new = function()
	local mesh = {}
	mesh.color = vmath.vector4(1.0, 1.0, 1.0, 1.0)

	mesh.apply_armature = function() --TODO: move to GPU?
		if not mesh.bones then
			return
		end

		for _, vertex in ipairs(mesh.vertices) do
			local pos = vmath.vector4()
			for _, group in ipairs(vertex.w) do
				local m = mesh.bones[group.bone_idx]
				local v =  m * vmath.vector4(vertex.p.x, vertex.p.y, vertex.p.z, 1.0)
				pos = pos + v * group.weight
			end
			vertex.p = vmath.vector3(pos.x, pos.y, pos.z)
		end
		
	end
	
	mesh.create_buffer = function()
		local buf = buffer.create(#mesh.faces * 3, {
			{name = hash("position"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("normal"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("texcoord0"), type = buffer.VALUE_TYPE_FLOAT32, count = 2}
		})
		local positions = buffer.get_stream(buf, "position")
		local normals = buffer.get_stream(buf, "normal")

		mesh.apply_armature()

		local count = 1
		for i, face in ipairs(mesh.faces) do
			for _, idx in ipairs(face.v) do
				positions[count] = mesh.vertices[idx].p.x
				positions[count + 1] = mesh.vertices[idx].p.y
				positions[count + 2] = mesh.vertices[idx].p.z

				local n = face.n or mesh.vertices[idx].n
				normals[count] = n.x
				normals[count + 1] = n.y
				normals[count + 2] = n.z
				count = count + 3
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