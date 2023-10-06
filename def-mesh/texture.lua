local function next_pot(num)
	local result = 1
	while num > result do
		result = bit.lshift(result, 1)
	end
	return result
end

function create_animation_texture(mesh)
	pprint("creating animation texture for " .. mesh.name .. " " .. next_pot(#mesh.frames) .. "x" .. next_pot(#mesh.frames[1]))

	local path = "/__anim_" .. mesh.name ..".texturec"
	local texture_id = hash(path)
	local tparams = {
		width          = next_pot(#mesh.frames[1]),
		height         = next_pot(#mesh.frames),
		type           = resource.TEXTURE_TYPE_2D,
		format         = resource.TEXTURE_FORMAT_RGBA32F,
	}
	local tbuffer = buffer.create(tparams.width * tparams.height, { {name=hash("rgba"), type=buffer.VALUE_TYPE_FLOAT32, count=4} } )

	local tstream = buffer.get_stream(tbuffer, hash("rgba"))

	local index = 1
	for f = 1, tparams.height do
		if f <= #mesh.frames then
			mesh.cache.bones = mesh.frames[f]
			mesh.calculate_bones()
			local bones = mesh.cache.calculated
			for b = 1, #bones do
				tstream[index + 0] = bones[b].x
				tstream[index + 1] = bones[b].y
				tstream[index + 2] = bones[b].z
				tstream[index + 3] = bones[b].w 
				index = index + 4
			end
			index = index + 4 * (tparams.width - #bones)
		end
	end
	
	
	if not pcall(function()
		texture_id = resource.create_texture(path, tparams, tbuffer)
	end) then
		--already exists
	end

	return texture_id, tparams.width, tparams.height
end