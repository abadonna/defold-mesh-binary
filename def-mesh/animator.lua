local M = {}

RM_NONE = 0
RM_ROTATION = 1
RM_POSITION = 2
RM_BOTH = 3
RM_FORWARD = 4

local function update_blend(self, dt)

	if self.blend then
		self.blend.factor = math.max(0, 1.0 - self.total_time / self.blend.duration)
		if self.blend.factor == 0 then
			if self.blend.animation and self.blend.animation.callback and not self.blend.animation.is_completed then
				self.blend.animation.callback(self.blend.animation.frame)
			end
			self.blend = nil
		elseif self.blend.animation then
			self.blend.animation:update(dt)
		end
		self.total_time = self.total_time + dt
	end

end

local function single_frame_update(self, dt)
	if self.is_completed then return end

	update_blend(self, dt)

	self.changed = true

	if not self.blend then
		self.is_completed = true
	end

	if (self.time == 0) and (self.motion > RM_NONE)  then
		self.reset_root_motion()
	end

	self.time = self.time + dt
end

local function animation_update(self, dt)
	if self.is_completed then return end

	update_blend(self, dt)

	local a = self.time / self.duration
	local full, part = math.modf(a)

	local frame = self.start + math.floor(self.length * part)
	
	if full >= 1 then --TODO loop
		frame = self.start + 1 -- skip first frame on loop
		self.time = 2 / self.fps -- self.time - self.duration * full
	end

	
	if (frame == self.finish or full >= 1) and self.playback == go.PLAYBACK_ONCE_FORWARD then
		self.is_completed = true
	end

	if frame ~= self.frame then
		self.frame = frame
		self.changed = true
	end

	if ((frame == self.start) or (full >=1) ) and (self.changed) and (self.motion > RM_NONE) then
		self.reset_root_motion()
	end

	self.time = self.time + dt
end

M.create = function(binary)
	local animator = {
		binary = binary,
		list = {},
		animations = {},
		frame = {[0]= {idx = 0, motion = RM_NONE} },
		tracks = {[0]={weight=1}}
	}

	animator.set_frame = function(track, frame1, frame2, blend, rm1, rm2)
		animator.frame[track] = {idx = frame1, motion = rm1 or RM_NONE}
		--pprint(frame1 .. ":" .. frame2 .. ", rm: " .. rm1 ..  ", " .. rm2)
		animator.binary:set_frame(track, frame1, frame2 == nil and -1 or frame2, blend or 0, rm1 or RM_NONE, rm2 or RM_NONE)
	end
 
	animator.update_tracks = function()
		animator.binary:update()
	end

	animator.stop = function(track)
		for i, a in ipairs(animator.animations) do
			if a.track == track then
				if a.callback then
					a.callback(false)
				end
				table.remove(animator.animations, i)
				break
			end
		end
	end

	animator.play = function(animation, config, callback)
		--[[
			config:
				track - track id
				blend_duration - seconds to blend with previous animation
				fps - frames per second
				playback - once or looped
				root_motion- RM_ROTATION, RM_POSITION, RM_BOTH, RM_FORWARD

		--]]
		if type(animation) == "string" then
			animation = {
				start = animator.list[animation].start,
				finish = animator.list[animation].finish
			}
		else
			animation = {
				start = animation.start,
				finish = animation.finish
			}
		end

		config = config or {}
		local blend_duration = config.blend_duration or 0

		animation.track = config.track or 0
		animation.fps = config.fps or 30
		animation.callback = callback
		animation.length = animation.finish - animation.start + 1
		animation.duration = animation.length / animation.fps
		animation.playback = config.playback or go.PLAYBACK_ONCE_FORWARD
		--so far supported only PLAYBACK_LOOP_FORWARD & PLAYBACK_ONCE_FORWARD
		animation.motion = config.root_motion or RM_NONE
		animation.primary = true

		animation.reset_root_motion = function() 
			if animation.track > 0 then return end -- TODO
			animator.binary:reset_root_motion(animation.primary, animation.start)
		end

		animation.time = 0
		animation.total_time = 0
		animation.update = animation_update

		if animation.start == animation.finish then -- just set or blend to frame
			animation.frame = animation.start
			animation.update = single_frame_update
			if animation.track == 0 then
				animator.binary:switch_root_motion()
			end
		end

		for i, a in ipairs(animator.animations) do
			if a.track == animation.track then
				a.blend = nil -- blend only 2 animations
				a.primary = false
				if animation.track == 0 then
					animator.binary:switch_root_motion()
				end
				if blend_duration > 0 then
					animation.blend = {
						animation = a,
						duration = blend_duration
					}
				end

				table.remove(animator.animations, i)
				break
			end
		end

		if blend_duration > 0 and not animation.blend then --blend with current frame
			if animation.track == 0 then
				animator.binary:switch_root_motion()
			end
			animation.blend = {
				frame = animator.frame[animation.track] or animator.frame[0],
				duration = blend_duration
			}
		end

		table.insert(animator.animations, animation)
	end

	animator.update = function(dt)
		local completed = {}

		for i = #animator.animations, 1, -1 do
			local a = animator.animations[i]
			a:update(dt)
			if a.changed then
				a.changed = false
				local blend_frame_idx = -1
				local blend_motion = RM_NONE
				if a.blend then
					blend_frame_idx = a.blend.frame and a.blend.frame.idx or a.blend.animation.frame
					blend_motion = a.blend.frame and a.blend.frame.motion or a.blend.animation.motion
				end
				animator.set_frame(a.track, a.frame, blend_frame_idx,
					a.blend and a.blend.factor or 0, 
					a.motion, blend_motion)
			end
			if a.is_completed then
				table.remove(animator.animations, i)
				table.insert(completed, a)
			end
		end

		for track_id, data in pairs(animator.tracks) do
			if data.duration then --animate weight
				data.time = data.time + dt
				local a = data.time / data.duration
				local full, part = math.modf(a)
				local length = data.finish - data.start
				local weight = data.start + length * part

				if full > 0 then
					data.duration = nil
					animator.set_weight(track_id, data.finish)
				else
					animator.set_weight(track_id, weight)
				end
			end
		end

		animator.update_tracks()

		for _, a in ipairs(completed) do
			if a.callback then a.callback(a.frame) end
		end

	end

	animator.add_track = function(mask, weight)
		local id = animator.binary:add_animation_track(mask)
		animator.tracks[id] = {}
		if weight and weight < 1 then
			animator.set_weight(id, weight)
		end
		return id
	end

	animator.set_weight = function(track_id, weight, duration)
		if duration then
			animator.tracks[track_id] = {
				start = animator.tracks[track_id].weight or 1,
				duration = duration,
				finish = weight,
				time = 0
			}

			return
		end
		animator.tracks[track_id].weight = weight
		animator.binary:set_animation_track_weight(track_id, weight)
	end

	animator.set_fps = function(track, fps)
		for i, a in ipairs(animator.animations) do
			if a.track == track then
				a.fps = fps
				a.duration = a.length / a.fps
			end
		end
	end

	return animator
end


return M