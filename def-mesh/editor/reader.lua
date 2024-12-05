local M = {}


M.init = function(path)
	local f = io.open(path, "r")
	M.content = f:read("*a")
	M.index = 1
	io.close(f)
end

M.read_materials = function()
	local materials = {}

	M.half_precision = M.read_int() == 1
	
	--skip armatures
	for i = 1, M.read_int()  do
		local count = M.read_int()
		for bone = 1, count do
			M.read_string()
			M.read_int()
			M.read_vec4()
			M.read_vec4()
			M.read_vec4()
		end

		for frame = 1, M.read_int() do
			for bone = 1, count do
				M.read_vec4()
				M.read_vec4()
				M.read_vec4()
			end
		end
	end

	while not M.eof() do
		M.read_model(materials)
	end
	return materials
end

M.read_model = function(materials)
	M.read_string() -- model name
	local parent = M.read_int()
	if parent > 0 then
		M.read_string(parent)
	end

	M.read_vec3() -- transform
	M.read_vec3()
	M.read_vec3()

	local vert_count = M.read_int()
	for vertex = 1, vert_count do --vertices
		M.read_vec3_hp()
		M.read_vec3_hp()
	end

	local shape_count = M.read_int()
	for shape = 1, shape_count do --shapes
		M.read_string()
		for delta = 1, M.read_int() do
			M.read_int()
			M.read_vec3()
			M.read_vec3()
		end
	end

	local count = M.read_int()
	for face = 1, count do --faces
		M.read_int()
		M.read_int()
		M.read_int()
		M.read_int()
		local flat = M.read_int()
		if flat == 1 then
			M.read_vec3()
		end
	end

	for face = 1, count * 6 do --uv
		M.read_float()
	end

	for material = 1, M.read_int() do --materials
		local name =  M.read_string()
		local type = M.read_int()
		table.insert(materials, {name = name, type = (type == 0 and "model" or "transparent")})
		M.read_vec4() -- color
		M.read_float() -- specular
		M.read_float() -- roughness
		local flag = M.read_int() 
		if flag > 0 then -- texture
			M.read_string(flag)
		end
		flag = M.read_int()
		if flag > 0 then -- normal texture
			M.read_string(flag)
			M.read_float()
		end
		flag = M.read_int()
		if flag > 0 then -- specular texture
			M.read_string(flag)
			M.read_int() 
			flag = M.read_int()
			if flag > 0 then  --ramp
				M.read_float()
				M.read_float()
				M.read_float()
				M.read_float()
			end
		end
		flag = M.read_int()
		if flag > 0 then -- roughness texture
			M.read_string(flag)
			M.read_int() 
			flag = M.read_int()
			if flag > 0 then  --ramp
				M.read_float()
				M.read_float()
				M.read_float()
				M.read_float()
			end
		end

	end

	if M.read_int() > -1 then --skin
		for vertex = 1, vert_count do
			for w = 1, M.read_int() do
				M.read_int()
				M.read_float()
			end
		end
	end

	for frame = 1, M.read_int() do --frames
		for i = 1, shape_count do
			M.read_string()
			M.read_float()
		end
	end

end

M.read_bones = function()
	M.half_precision = M.read_int() == 1
	
	local count = M.read_int() -- number of armatures
	count = M.read_int()  -- number of bones in first armature

	local bones = {}
	for i = 1, count do
		table.insert(bones, {name = M.read_string(), enabled = true})
		M.read_int()  --parent
		M.read_vec4()
		M.read_vec4()
		M.read_vec4()
	end

	return bones

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

M.read_float_hp = function()
	M.index = M.index + 2
	return 0
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
	return {M.read_float(), M.read_float(), M.read_float()}
end

M.read_vec3_hp = function()
	if M.half_precision then
		return {M.read_float_hp(), M.read_float_hp(), M.read_float_hp()}
	end
	return {M.read_float(), M.read_float(), M.read_float()}
end

M.read_vec4 = function()
	return {M.read_float(), M.read_float(), M.read_float(), M.read_float()}
end


return M