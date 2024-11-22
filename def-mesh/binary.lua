local ANIMATOR = require "def-mesh.animator"

local M = {}

M.BUFFER_COUNT = 0

local function create_buffer(buf)
	M.BUFFER_COUNT = M.BUFFER_COUNT and M.BUFFER_COUNT + 1 or 1
	local path = "/def-mesh/buffers/buffer" .. M.BUFFER_COUNT .. ".bufferc"
	return resource.create_buffer(path, {buffer = buf, transfer_ownership = false}), path
end

function native_update_buffer(mesh_url, buffer)
	resource.set_buffer(go.get(mesh_url, "vertices"), buffer, {transfer_ownership = false})
end

function native_runtime_texture(tpath, width, height, buffer)
	local tparams = {
		width          = width,
		height         = height,
		type           = resource.TEXTURE_TYPE_2D,
		format         = resource.TEXTURE_FORMAT_RGBA32F,
	}
	resource.set_texture(tpath, tparams, buffer)
end

local function get_animation_texture(path, model, runtime)

	local tpath = (runtime and path or string.match(path, "(%a-)[$%.]")) .. "_" .. model.armature
	tpath = "/__anim_" .. tpath:gsub("[%./]", "") ..".texturec"

	if not pcall(function()
		resource.get_texture_info(tpath)
	end) then
		local twidth, theight, tbuffer = model:get_animation_buffer(runtime)
		local tparams = {
			width          = twidth,
			height         = theight,
			type           = resource.TEXTURE_TYPE_2D,
			format         = resource.TEXTURE_FORMAT_RGBA32F,
		}

		return resource.create_texture(tpath, tparams, tbuffer)
	else
		return hash(tpath)
	end
end

local function set_texture(self, url, slot, file, texel)
	if not file then
		return false
	end
	local data = sys.load_resource(self.texture_folder .. file)
	if not data then
		pprint(self.texture_folder .. file .. " not found")
		return false
	end

	local path = self.texture_folder .. file ..".texturec"
	local img = imageloader.load{data = data}

	local texture_id = hash(path)
	if not pcall(function()
		texture_id = resource.create_texture(path, img.header, img.buffer)
	end) then
		--already exists?
	end
	self.textures[texture_id] = true
	go.set(url, slot, texture_id)
	if texel then
		go.set(url, texel, vmath.vector4(1./img.header.width, 1./img.header.height, 0, 0))
	end
	return true
end

local function get_bone_go(self, bone)
	if self.bones_go[bone] then
		return self.bones_go[bone]
	end

	local id = factory.create(self.url .. "#factory_bone")

	local parent = self.binary:attach_bone_go(id, bone)
	if parent then
		go.set_parent(id, parent)
		self.bones_go[bone] = id

		return id
	end

	return nil
end

--[[
url - url to binary.go instance
path - path to .bin file in custom assets
config = {
	textures - path to folder with textures
	bake -  true to bake animations into texture
	materials - table of materials to replace, 
		use editor script "Add materials from model" to generate properties
	}
--]]
M.load = function(url, path, config)
	local instance = {
		meshes = {},
		attaches = {},
		bones_go = {},
		textures = {},
		game_objects = {},
		url = url,
		uid = math.random(0, 10000000)
	}

	config = config or {}

	instance.texture_folder = config.textures or "/assets/"
	if string.find(instance.texture_folder, "/") ~= 1 then
		instance.texture_folder = "/" .. instance.texture_folder
	end
	if string.find(instance.texture_folder, "/$") == nil then
		instance.texture_folder = instance.texture_folder .. "/"
	end

	local models
	local data = sys.load_resource(path)
	instance.binary, models = mesh_utils.load(path, data, config.bake or false)
	instance.animator = ANIMATOR.create(instance.binary)
	
	for name, model in pairs(models) do 

		instance.total_frames = model.frames

		local anim_texture, runtime_texture

		if model.frames > 1 and config.bake then
			anim_texture = get_animation_texture(path, model)
			runtime_texture = get_animation_texture(instance.uid, model, true)
			model:set_runtime_texture(runtime_texture)
		end

		for i, mesh in ipairs(model.meshes) do
			
			--local f = mesh.material.type == 0 and "#factory" or "#factory_trans"
			--local id = factory.create(url .. f, model.position, model.rotation, {}, model.scale)
			
			local id = factory.create(url .. "#factory", model.position, model.rotation, {}, model.scale)
			local mesh_url = msg.url(nil, id, "mesh")
			mesh:set_url(mesh_url);

			if config.materials and config.materials[mesh.material.name] then
				go.set(mesh_url, "material", config.materials[mesh.material.name])
			elseif mesh.material.type > 0 then
				go.set(mesh_url, "material", go.get(url .. "#binary", "transparent"))
			end
			
			local v, bpath = create_buffer(mesh.buffer)
			go.set(mesh_url, "vertices", v)

			instance.game_objects[i == 1 and name or (name .. "_" .. i)] = 
			{
				id = id, 
				url = mesh_url, 
				path = bpath,
				parent = model.parent
			}

			local options = vmath.vector4(0.0)
			local options_specular = vmath.vector4(0.0)
			options_specular.y = mesh.material.specular.value
			options_specular.z = mesh.material.roughness.value

			options.z = mesh.material.type

			if anim_texture then
				instance.textures[anim_texture] = true;
				instance.textures[runtime_texture] = true;
				go.set(mesh_url, "texture3", anim_texture)
				go.set(mesh_url, "texture4", runtime_texture)
			end

			if mesh.material.texture then
				set_texture(instance, mesh_url, "texture0", mesh.material.texture, "texel")
				options.x = 1.0
			end

			if mesh.material.normal and
			set_texture(instance, mesh_url, "texture1", mesh.material.normal.texture) 
			then
				options.y = mesh.material.normal.value
			end

			if mesh.material.specular.texture and
			set_texture(instance, mesh_url, "texture2", mesh.material.specular.texture) 
			then
				options_specular.x = 1.0 + mesh.material.specular.invert
				if mesh.material.specular.ramp then
					local r = mesh.material.specular.ramp
					go.set(mesh_url, "spec_ramp", vmath.vector4(r.p1, r.v1, r.p2, r.v2))
				end
			end

			if mesh.material.roughness.texture and
			set_texture(instance, mesh_url, "texture2", mesh.material.roughness.texture) 
			then
				-- roughness and specular are usually the same?
				options_specular.w = 1.0
				if mesh.material.roughness.ramp then
					local r = mesh.material.roughness.ramp
					go.set(mesh_url, "rough_ramp", vmath.vector4(r.p1, r.v1, r.p2, r.v2))
				end
			end

			go.set(mesh_url, "base_color", mesh.material and mesh.material.color or vmath.vector4(0.8,0.8,0.8,1))
			go.set(mesh_url, "options", options)
			go.set(mesh_url, "options_specular", options_specular)
		end

	end	


	--set hierarchy
	for _, mesh in pairs(instance.game_objects) do
		if mesh.parent and instance.game_objects[mesh.parent] then
			go.set_parent(mesh.id, instance.game_objects[mesh.parent].id, false)			
		else
			go.set_parent(mesh.id, url, false)
		end
	end

	instance.binary:set_frame(0, 0);
	instance.binary:update();

	---------------------------------------------------------------

	instance.delete = function(with_objects)
		if with_objects then
			go.delete(instance.url, true)
		end
		
		for key, mesh in pairs(instance.game_objects) do
			resource.release(mesh.path)
		end

		if instance.binary then
			if instance.binary:delete() then --clean up textures, no more instances
				for texture, _ in pairs(instance.textures) do
					resource.release(texture)
				end
			end
		end

	end

	instance.set_frame = function(frame1, frame2, blend) -- use animator directly for more flexible approach 
		instance.animator.set_frame(0, frame1, frame2, blend)
		instance.animator.update_tracks()
	end

	instance.attach = function(bone, target_url)
		local id = get_bone_go(instance, bone)
		go.set_parent(target_url, id, true)
	end

	instance.set_shapes = function(shapes)
		instance.binary:set_shapes(shapes)
	end

	instance.set = function(property, value)
		for _, mesh in pairs(instance.game_objects) do
			go.set(mesh.url, property, value)
		end
	end

	return instance
end




return M