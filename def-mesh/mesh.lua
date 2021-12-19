local M = {}

M.new = function()
	local mesh = {}
	mesh.color = vmath.vector4(1.0, 1.0, 1.0, 1.0)
	mesh.create_buffer = function()
		local buf = buffer.create(#mesh.faces * 3, {
			{name = hash("position"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("normal"), type = buffer.VALUE_TYPE_FLOAT32, count = 3},
			{name = hash("texcoord0"), type = buffer.VALUE_TYPE_FLOAT32, count = 2}
		})
		local positions = buffer.get_stream(buf, "position")
		local normals = buffer.get_stream(buf, "normal")

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