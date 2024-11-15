local M = {}

local function animation_update(self, dt)
	if self.is_completed then return end
	
	self.time = self.time + dt
	local a = self.time / self.duration
	local full, part = math.modf(a)

	if full >= 1 then --TODO loop
		self.time = self.time - self.duration * full
	end

	local frame = self.start + math.floor(self.length * part)

	if (frame == self.finish or full >= 1) and self.playback == go.PLAYBACK_ONCE_FORWARD then
		if self.callback then self.callback(true) end
		self.is_completed = true
	end

	if self.blend then
		self.blend.factor = math.max(0, 1.0 - self.time / self.blend.duration)
		if self.blend.factor == 0 then
			if self.blend.animation and self.blend.animation.callback and not self.blend.animation.is_completed then
				self.blend.animation.callback(false)
			end
			self.blend = nil
		elseif self.blend.animation then
			self.blend.animation:update(dt)
		end
	end
	
	if frame ~= self.frame then
		self.frame = frame
		self.changed = true
	end
end

M.create = function(binary)
	local animator = {
		binary = binary,
		list = {},
		animations = {},
		frame = {[0]=0}
	}
	
	animator.set_frame = function(track, frame1, frame2, blend)
		animator.frame[track] = frame1
		animator.binary:set_frame(track, frame1, frame2 == nil and -1 or frame2, blend or 0)
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
		if type(animation) == "string" then
			animation = {
				start = animator.list[animation].start,
				finish = animator.list[animation].finish
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
		
		animation.time = 0
		animation.update = animation_update
		
		for i, a in ipairs(animator.animations) do
			if a.track == animation.track then
				a.blend = nil -- blend only 2 animations
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
			animation.blend = {
				frame = animator.frame[animation.track] or animator.frame[0],
				duration = blend_duration
			}
		end

		table.insert(animator.animations, animation)
	end

	animator.update = function(dt)

		for i = #animator.animations, 1, -1 do
			local a = animator.animations[i]
			a:update(dt)
			if a.changed then
				a.changed = false
				animator.set_frame(a.track, a.frame, 
					a.blend and (a.blend.frame or a.blend.animation.frame) or -1, 
					a.blend and a.blend.factor or 0)
			end
			if a.is_completed then
				table.remove(animator.animations, i)
			end
		end

		animator.update_tracks()

	end

	animator.add_track = function(mask, weight)
		local id = animator.binary:add_animation_track(mask)
		if weight and weight < 1 then
			animator.binary:set_animation_track_weight(id, weight)
		end
		return id
	end

	animator.set_weight = function(track, weight)
		animator.binary:set_animation_track_weight(track, weight)
	end
	
	return animator
end


return M