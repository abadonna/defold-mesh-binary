local M = {}


M.init = function(path)
	local f = io.open(path, "r")
	M.content = f:read("*a")
	M.index = 1
	io.close(f)
end

M.read_armature = function()
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

M.read_vec4 = function()
	return {M.read_float(), M.read_float(), M.read_float(), M.read_float()}
end


return M