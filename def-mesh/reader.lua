local M = {}

M.init_from_resource = function(path)
	M.content = sys.load_resource(path)
	M.index = 1
end

M.read_mesh = function()
	local mesh = {color = vmath.vector4(1.0, 1.0, 1.0, 1.0)}

	mesh.name = M.read_string()
	local parent_flag = M.read_int()
	if parent_flag > 0 then
		mesh.parent =  M.read_string(parent_flag)
	end
	mesh.position = vmath.vector3(M.read_float(), M.read_float(), M.read_float())

	local qx =  vmath.quat_rotation_x(M.read_float())
	local qy =  vmath.quat_rotation_y(M.read_float())
	local qz =  vmath.quat_rotation_z(M.read_float())
	mesh.rotation = qx * qy * qz

	local vertex_count = M.read_int()
	mesh.vertices = {}
	for i = 1, vertex_count do
		table.insert(mesh.vertices, 
		{
			p = {
				x = M.read_float(), 
				y = M.read_float(), 
				z = M.read_float()
			}
		})
	end

	for i = 1, vertex_count do
		mesh.vertices[i].n = 
		{
			x = M.read_float(), 
			y = M.read_float(), 
			z = M.read_float()
		}
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
			}
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
		mesh.faces[flat_faces[i]].n = {
			x = M.read_float(), 
			y = M.read_float(), 
			z = M.read_float()
		}
	end

	local material_flag = M.read_int()
	if material_flag == 1 then
		mesh.color = vmath.vector4(M.read_float(), M.read_float(), M.read_float(), M.read_float())
		local texture_flag = M.read_int()
		if texture_flag > 0 then
			mesh.texture = M.read_string(texture_flag)
			--pprint(mesh.texture)
		end
	end
	
	return mesh
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


return M