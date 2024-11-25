#include "animation.h"

Animation::Animation(Armature* armature) {
	this->armature = armature;
	AnimationTrack base;
	this->tracks.reserve(8); //to avoid losing pointers to calculated bones
	this->tracks.push_back(base);
	this->SetFrame(0, 0, -1, 0, false, false);
	this->Update(0, false, false);

}

void Animation::SetTransform(Matrix4* matrix, int frame) {
	this->transform = (matrix != NULL) ? *matrix : this->transform;
	
	Matrix4 m = this->armature->frames[frame][this->armature->rootBoneIdx];
	Matrix4 local = this->armature->localBones[this->armature->rootBoneIdx];
	this->root_transform = this->transform * Transpose(Inverse(local) * m * local) * Inverse(this->transform);
}

bool Animation::IsBlending() {
	int count = 0;
	for (AnimationTrack & track : this->tracks) {
		if (track.weight > 0) count++;
		if (track.frame2 > -1) return true;
	}
	return count > 1;
}

void Animation::Update(dmGameObject::HInstance root, bool rotation, bool position) {
	if (!this->needUpdate) return;
	
	this->bones = this->tracks[0].bones; 

	int count = 0;
	for (AnimationTrack & track : this->tracks) {
		if ((track.weight > 0) && (track.frame1 > -1)) {
			count ++;
			if (count == 1) { 
				this->bones = track.bones; 
				continue;
			}

			if (count == 2) {//copy bones from base track
				this->cumulative = *this->bones; 
				this->bones = &this->cumulative; 
			}

			for (int & idx : track.mask) {
				if (track.weight == 1.0) {
					this->cumulative[idx] = track.bones->at(idx);
				}else {
					MatrixBlend(&this->cumulative, track.bones, &this->cumulative, idx, track.weight);
				}
			}

		}
	}

	this->CalculateBones(root, rotation, position);

}


void Animation::CalculateBones(dmGameObject::HInstance root, bool applyRotation, bool applyPosition) {
	if (this->bones == NULL) return;

	int size = this->bones->size();
	if (this->cumulative.size() < size) { //never acumulated, just copy to expand
		this->cumulative = *this->bones;
	}

	bool has_parent_transforms[size];

	for (int idx = 0; idx < size; idx ++) { //precalculate parent transforms
		Matrix4 local = this->armature->localBones[idx];
		this->cumulative[idx] = Inverse(local) * this->bones->at(idx) * local;


		if ((root != 0) && (applyRotation || applyPosition) && (this->armature->rootBoneIdx == idx)) {
			
			Vector4 posePosition = this->cumulative[idx].getRow(3);
			Matrix4 worldRootBone = this->transform * Transpose(this->cumulative[idx]) * Inverse(this->transform);
			Quat rotation = dmGameObject::GetRotation(root);
			
			if (applyRotation) {
				Matrix4 m = worldRootBone * Inverse(this->root_transform);
				Quat q = Quat(m.getUpper3x3());
				Quat diff = Quat(q.getX(), q.getZ(), -q.getY(), q.getW());
	
				dmGameObject::SetRotation(root, rotation * diff);

				this->cumulative[idx].setCol0(Vector4(1, 0, 0, posePosition[0]));
				this->cumulative[idx].setCol1(Vector4(0, 1, 0, posePosition[1]));
				this->cumulative[idx].setCol2(Vector4(0, 0, 1, posePosition[2]));

			} 

			if (applyPosition) {
				Vector4 v = worldRootBone.getCol3() - this->root_transform.getCol3();
				Vector3 diff = Vector3(v.getX(), /*v.getZ()*/ 0, -v.getY()); //only xz movement, Unity way
				diff = dmVMath::Rotate(rotation, diff);

				Point3 position = dmGameObject::GetPosition(root);

				dmGameObject::SetPosition(root, position + diff);
				this->cumulative[idx].setRow(3, Vector4(0, posePosition[1], 0, 1));
			}

			this->root_transform = worldRootBone; //for the next frame
			
		}
		has_parent_transforms[idx] = false;
	}


	for (int idx = 0; idx < size; idx ++) {
		Matrix4 bone = this->cumulative[idx];

		int parent = this->armature->boneParents[idx];
		while (parent > -1) {
			bone = bone * this->cumulative[parent];
			if (has_parent_transforms[parent]) break; //optimization
			parent = this->armature->boneParents[parent];
		}

		has_parent_transforms[idx] = true;
		this->cumulative[idx] = bone;
	}

	this->bones = &this->cumulative;
	this->needUpdate = false;
}

void Animation::SetFrame(int trackIdx,  int idx1, int idx2, float factor, bool useBakedAnimations, bool hasAttachaments) {
	if (trackIdx >= this->tracks.size()) { return; }

	int last_frame = this->armature->frames.size() - 1;
	AnimationTrack* track = &this->tracks[trackIdx];

	idx1 = (idx1 < last_frame) ? idx1 : last_frame;
	idx2 = (idx2 < last_frame) ? idx2 : last_frame;

	bool hasChanged = (track->frame1 != idx1 || track->frame2 != idx2 || track->factor != factor);

	track->frame1 = idx1;
	track->frame2 = idx2;
	track->factor = factor;

	if (hasChanged) {
		this->needUpdate = true;
		if (idx2 > -1) {
			track->Interpolate(this->armature);
		} else {
			track->bones = &this->armature->frames[idx1];
		}
	} 
}

int Animation::GetRuntimeBuffer(lua_State* L) {	

	int width = this->armature->animationTextureWidth;
	int height = 2;

	const dmBuffer::StreamDeclaration streams_decl[] = {
		{dmHashString64("rgba"), dmBuffer::VALUE_TYPE_FLOAT32, 4}
	};

	dmBuffer::HBuffer buffer = 0x0;
	dmBuffer::Create(width * height, streams_decl, 1, &buffer);

	float* stream = 0x0;

	uint32_t components = 0;
	uint32_t stride = 0;
	uint32_t items_count = 0;

	dmBuffer::GetStream(buffer, dmHashString64("rgba"), (void**)&stream, &items_count, &components, &stride);

	if (this->bones == NULL) {
		this->bones = &this->armature->frames[0];
		this->CalculateBones(0, false, false);
	}

	for(auto & bone : *this->bones) {
		Vector4 data[3] = {bone.getCol0(), bone.getCol1(), bone.getCol2()};
			for (int i = 0; i < 3; i++) {
				stream[0] = data[i].getX();
				stream[1] = data[i].getY();
				stream[2] = data[i].getZ();
				stream[3] = data[i].getW();
				stream += stride;
			}
		}

	lua_pushnumber(L, width);
	lua_pushnumber(L, height);

	dmScript::LuaHBuffer luabuf(buffer, dmScript::OWNER_LUA);
	dmScript::PushBuffer(L, luabuf);

	return 3;
}

int Animation::GetTextureBuffer(lua_State* L) {	
	int frameCount = this->armature->frames.size();

	int width = this->armature->animationTextureWidth;
	int height = this->armature->animationTextureHeight;

	const dmBuffer::StreamDeclaration streams_decl[] = {
		{dmHashString64("rgba"), dmBuffer::VALUE_TYPE_FLOAT32, 4}
	};

	dmBuffer::HBuffer buffer = 0x0;
	dmBuffer::Create(width * height, streams_decl, 1, &buffer);

	float* stream = 0x0;

	uint32_t components = 0;
	uint32_t stride = 0;
	uint32_t items_count = 0;

	dmBuffer::GetStream(buffer, dmHashString64("rgba"), (void**)&stream, &items_count, &components, &stride);

	for (int f = 0; f < height; f++) {
		if (f < frameCount) {
			this->bones = &this->armature->frames[f];
			this->CalculateBones(0, false, false);

			for(auto & bone : *this->bones) {
				Vector4 data[3] = {bone.getCol0(), bone.getCol1(), bone.getCol2()};
				for (int i = 0; i < 3; i++) {
					stream[0] = data[i].getX();
					stream[1] = data[i].getY();
					stream[2] = data[i].getZ();
					stream[3] = data[i].getW();
					stream += stride;
				}
			}

			stream += stride * (width - this->bones->size() * 3);
		}
	}

	this->bones = &this->armature->frames[0];
	this->CalculateBones(0, false, false); // return to first frame

	lua_pushnumber(L, width);
	lua_pushnumber(L, height);

	dmScript::LuaHBuffer luabuf(buffer, dmScript::OWNER_LUA);
	dmScript::PushBuffer(L, luabuf);

	return 3;
}

Vector4 Animation::GetBakedUniform() {
	if (this->IsBlending()) {
		return Vector4(1.0 / this->armature->animationTextureWidth, 0, 0, 1);
	}
	return Vector4(1.0 / this->armature->animationTextureWidth, (float)this->tracks[0].frame1 / this->armature->animationTextureHeight, 0, 0);
}

int Animation::FindBone(string bone) {
	return this->armature->FindBone(bone);
}

int Animation::GetFramesCount() {
	return this->armature->GetFramesCount();
}

int Animation::AddAnimationTrack(vector<string>* mask) {

	AnimationTrack track;

	for (auto & bone : *mask) {
		int idx = this->armature->FindBone(bone);
		if (idx > -1) {
			track.mask.push_back(idx);
		}
	}

	this->tracks.push_back(track);
	return this->tracks.size() - 1;
}

void Animation::SetTrackWeight(int idx, float weight) {
	if (this->tracks.size() > idx) {
		this->needUpdate = true;
		this->tracks[idx].weight = weight;
	}
}

int Animation::GetFrameIdx() {
	return this->tracks[0].frame1;
}