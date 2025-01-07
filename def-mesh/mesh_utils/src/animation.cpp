#include "animation.h"

Animation::Animation(Armature* armature, dmGameObject::HInstance obj) {
	this->armature = armature;
	this->root = obj;
	AnimationTrack base = AnimationTrack(armature, NULL);
	this->tracks.reserve(8); //to avoid losing pointers to calculated bones
	this->tracks.push_back(base);
	this->SetFrame(0, 0, -1, 0, RootMotionType::None, RootMotionType::None);
	this->Update();

}

void Animation::SwitchRootMotion() {
	this->tracks[0].SwitchRootMotion();
}

void Animation::ResetRootMotion(int frameIdx, bool isPrimary) {
	this->tracks[0].ResetRootMotion(frameIdx, isPrimary);
}

void Animation::SetTransform(Matrix4 matrix) {
	this->transform = matrix;
	this->tracks[0].SetTransform(matrix);
}

bool Animation::IsBlending() {
	int count = 0;
	for (AnimationTrack & track : this->tracks) {
		if (track.weight > 0) count++;
		if (track.frame2 > -1) return true;
	}
	return count > 1;
}

bool Animation::HasRootMotion() {
	return this->hasRootMotion;
}

void Animation::Update() {
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
					MatrixBlend(&this->cumulative[idx], &track.bones->at(idx), &this->cumulative[idx], track.weight);
				}
			}

		}
	}

	this->CalculateBones();

	for(auto & obj : this->boneObjects) {
		obj.ApplyTransform(this->bones);
	}
}


void Animation::CalculateBones() {
	if (this->bones == NULL) return;

	int size = this->bones->size();
	if (this->cumulative.size() < size) { //never acumulated, just copy to expand
		this->cumulative = *this->bones;
	}

	bool has_parent_transforms[size];

	for (int idx = 0; idx < size; idx ++) { //precalculate parent transforms
		Matrix4 local = this->armature->localBones[idx];
		this->cumulative[idx] = local * this->bones->at(idx) * Inverse(local);

		has_parent_transforms[idx] = false;
	}


	for (int idx = 0; idx < size; idx ++) {
		Matrix4 bone = this->cumulative[idx];

		int parent = this->armature->boneParents[idx];
		while (parent > -1) {
			bone = this->cumulative[parent] * bone;
			if (has_parent_transforms[parent]) break; //optimization
			parent = this->armature->boneParents[parent];
		}

		has_parent_transforms[idx] = true;
		this->cumulative[idx] = bone;
	}

	this->bones = &this->cumulative;
	this->needUpdate = false;
}

void Animation::SetFrame(int trackIdx,  int idx1, int idx2, float factor, RootMotionType rm1, RootMotionType rm2) {
	if (trackIdx >= this->tracks.size()) { return; }

	int last_frame = this->armature->frames.size() - 1;
	AnimationTrack* track = &this->tracks[trackIdx];

	idx1 = (idx1 < last_frame) ? idx1 : last_frame;
	idx2 = (idx2 < last_frame) ? idx2 : last_frame;

	factor = (idx2 > -1) ? factor : 0;

	bool hasChanged = ((track->frame1 != idx1) || (track->frame2 != idx2) || fabs(track->factor - factor) > 0.00001);

	track->frame1 = idx1;
	track->frame2 = idx2;
	track->factor = factor;

	if (hasChanged) {
		this->needUpdate = true;

		if (idx2 > -1) {
			track->Interpolate();
		} else {
			track->bones = &this->armature->frames[idx1];
		}

		if (trackIdx == 0) {
			track->ExtractRootMotion(this->root, rm1, rm2);
			this->hasRootMotion = (rm1 != RootMotionType::None) || (rm2 != RootMotionType::None);
		} //TODO: for all tracks
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
		this->CalculateBones();
	}

	for(auto & bone : *this->bones) {
		Vector4 data[3] = {bone.getRow(0), bone.getRow(1), bone.getRow(2)};
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
			this->CalculateBones();

			for(auto & bone : *this->bones) {
				Vector4 data[3] = {bone.getRow(0), bone.getRow(1), bone.getRow(2)};
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
	this->CalculateBones(); // return to first frame

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
	AnimationTrack track = AnimationTrack(this->armature, mask);
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

bool Animation::HasAttachments() {
	return this->boneObjects.size() > 0;
}


void Animation::CreateBoneGO(dmGameObject::HInstance go, int idx) {
	BoneGameObject obj = BoneGameObject(go, idx);
	this->boneObjects.push_back(obj);
	obj.ApplyTransform(this->bones);
}