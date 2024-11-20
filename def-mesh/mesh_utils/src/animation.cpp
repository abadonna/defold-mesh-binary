#include "animation.h"

Animation::Animation(Armature* armature) {
	this->armature = armature;
	AnimationTrack base;
	this->tracks.reserve(8); //to avoid losing pointers to calculated bones
	this->tracks.push_back(base);
}

void Animation::Update(lua_State* L) {

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

	this->CalculateBones();

}


void Animation::CalculateBones() {
	if (this->bones == NULL) return;

	//Matrix4 invLocal = dmVMath::Inverse(this->model->local.matrix);
	int size = this->bones->size();
	if (this->cumulative.size() < size) { //never acumulated, jsut copy to expand
		this->cumulative = *this->bones;
	}

	Matrix4 temp[size];
	bool has_parent_transforms[size];

	for (int idx = 0; idx < size; idx ++) { //precalculate parent transforms
		Matrix4 local = this->armature->localBones[idx];
		temp[idx] = dmVMath::Inverse(local) * this->bones->at(idx) * local;
		has_parent_transforms[idx] = false;
	}

	for (int idx = 0; idx < size; idx ++) {
		Matrix4 bone = temp[idx];

		int parent = this->armature->boneParents[idx];
		while (parent > -1) {
			bone = bone * temp[parent];
			if (has_parent_transforms[parent]) break; //optimization
			parent = this->armature->boneParents[parent];
		}

		has_parent_transforms[idx] = true;
		temp[idx] = bone;

		this->cumulative[idx] = bone;
		//this->cumulative[idx] = model->local.matrix * bone * invLocal;
	}

	this->bones = &this->cumulative;
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

	if (hasChanged && (!useBakedAnimations || hasAttachaments)) {
		if ((idx2 > -1) && (!useBakedAnimations)) {
			track->Interpolate(this->armature);
		} else {
			track->bones = &this->armature->frames[idx1];
		}
	} 
}

void Animation::GetTextureBuffer(lua_State* L, Matrix4 local) {
	//REFACTOR local matrix!
	Matrix4 inverted = dmVMath::Inverse(local);
	
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
				Matrix4 m = local * bone * inverted;
				Vector4 data[3] = {m.getCol0(), m.getCol1(), m.getCol2()};
				//Vector4 data[3] = {bone.getCol0(), bone.getCol1(), bone.getCol2()};
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
}

Vector4 Animation::GetBakedUniform() {
	return Vector4(1.0 / this->armature->animationTextureWidth, (float)this->tracks[0].frame1 / this->armature->animationTextureHeight, 0, 0);
}

int Animation::FindBone(string bone) {
	return this->armature->FindBone(bone);
}

int Animation::GetFramesCount() {
	return this->armature->GetFramesCount();
}

void Animation::AddAnimationTrack(vector<string>* mask) {

	AnimationTrack track;

	for (auto & bone : *mask) {
		int idx = this->armature->FindBone(bone);
		if (idx > -1) {
			track.mask.push_back(idx);
		}
	}

	this->tracks.push_back(track);
}

