#include "animation.h"

Animation::Animation(Armature* armature, dmGameObject::HInstance obj) {
	this->armature = armature;
	this->root = obj;
	AnimationTrack base;
	this->tracks.reserve(8); //to avoid losing pointers to calculated bones
	this->tracks.push_back(base);
	this->SetFrame(0, 0, -1, 0, RootMotion::None, RootMotion::None);
	this->Update();

}

void Animation::SetTransform(Matrix4* matrix, int frame1, int frame2) {
	this->transform = (matrix != NULL) ? *matrix : this->transform;

	int bi = this->armature->rootBoneIdx;

	Matrix4 local = this->armature->localBones[bi];
	Quat rotation = dmGameObject::GetRotation(this->root);
	
	if (frame1 > -1) {
		Matrix4 m = this->armature->frames[frame1][bi];
		m = Transpose(Inverse(local) * m * local);

		Vector4 v = this->transform * m.getCol3();
		this->position1 = Vector3(v.getX(), 0, -v.getY());
		this->position1 = dmVMath::Rotate(rotation, this->position1);
		
		m = this->transform * m * Inverse(this->transform);
		this->angle1 = QuatToEuler(Quat(m.getUpper3x3())).getZ();
	}
	
	if (frame2 > -1) {
		Matrix4 m = this->armature->frames[frame2][bi];
		m = Transpose(Inverse(local) * m * local);

		Vector4 v = this->transform * m.getCol3();
		this->position2 = Vector3(v.getX(), 0, -v.getY());
		this->position2 = dmVMath::Rotate(rotation, this->position2);

		m = this->transform * m * Inverse(this->transform);
		this->angle2 = QuatToEuler(Quat(m.getUpper3x3())).getZ();
	}
}

bool Animation::IsBlending() {
	int count = 0;
	for (AnimationTrack & track : this->tracks) {
		if (track.weight > 0) count++;
		if (track.frame2 > -1) return true;
	}
	return count > 1;
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

	this->CalculateBones(false, false);

}


void Animation::CalculateBones(bool applyRotation, bool applyPosition) {
	if (this->bones == NULL) return;

	int size = this->bones->size();
	if (this->cumulative.size() < size) { //never acumulated, just copy to expand
		this->cumulative = *this->bones;
	}

	bool has_parent_transforms[size];

	for (int idx = 0; idx < size; idx ++) { //precalculate parent transforms
		Matrix4 local = this->armature->localBones[idx];
		this->cumulative[idx] = Inverse(local) * this->bones->at(idx) * local;

/*
		if ((applyRotation || applyPosition) && (this->armature->rootBoneIdx == idx)) {

			Matrix4 m = Transpose(this->cumulative[idx]);
			
			Vector4 posePosition = this->cumulative[idx].getRow(3);
			Matrix4 worldRootBone = this->transform * m * Inverse(this->transform);

			Quat rotation = dmGameObject::GetRotation(this->root);

			
			if (applyRotation) {

				float angle = QuatToEuler(Quat(worldRootBone.getUpper3x3())).getZ();
				
				//dmLogInfo("%d, %f, %f, %f", this->GetFrameIdx(), angle1, angle2, angle1 - angle2)
				
				Quat diff = Quat::rotationY(angle - this->angle1);
				rotation = diff * rotation;
				dmGameObject::SetRotation(this->root, rotation);

				Matrix4 mm = Matrix4::rotationZ(angle);
				mm =  worldRootBone * Inverse(mm);
				mm = Inverse(this->transform) * mm * this->transform;
				
				Matrix3 mXZ = mm.getUpper3x3();
				this->cumulative[idx].setUpper3x3(Transpose(mXZ));

				mm = Matrix4::rotationZ(-angle);
				mm = Inverse(this->transform) * mm * this->transform;
				posePosition = mm * posePosition;
				
				this->cumulative[idx].setRow(3, posePosition);
				this->angle1 = angle;
			} 

			if (applyPosition) {

				Vector4 v = this->transform * posePosition;
				Vector3 position = Vector3(v.getX(), v.getZ(), -v.getY());
				position = dmVMath::Rotate(rotation, position);

				//------------remove Y ---------
				v = Inverse(this->transform) * Vector4(0, 0, position[1], 0);
				position[1] = 0;
				//------------------------------
				
				Point3 p = dmGameObject::GetPosition(this->root);
				dmGameObject::SetPosition(this->root, p + position - this->position1);
				
				this->cumulative[idx].setRow(3, v);
				this->position1 = position;

				//dmLogInfo("%d, %f, %f, %f", this->GetFrameIdx(), position[0],  position[1],  position[2]);
			}

			
		}*/
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

void Animation::SetFrame(int trackIdx,  int idx1, int idx2, float factor, RootMotion rm1, RootMotion rm2) {
	if (trackIdx >= this->tracks.size()) { return; }

	int last_frame = this->armature->frames.size() - 1;
	AnimationTrack* track = &this->tracks[trackIdx];

	idx1 = (idx1 < last_frame) ? idx1 : last_frame;
	idx2 = (idx2 < last_frame) ? idx2 : last_frame;

	bool hasChanged = ((track->frame1 != idx1) || (track->frame2 != idx2) || fabs(track->factor - factor) > 0.00001);

	track->frame1 = idx1;
	track->frame2 = idx2;
	track->factor = factor;

	if (hasChanged) {
		this->needUpdate = true;
		//dmLogInfo("setframe: %d, %d", track->frame1, track->frame2);
		if (idx2 > -1) {
			track->Interpolate(this->armature);
		} else {
			track->bones = &this->armature->frames[idx1];
		}

		if (trackIdx == 0) {ExtractRootMotion(rm1, rm2);} //TODO: for all tracks
	} 
}

void Animation::GetRootMotionForFrame(int idx, RootMotion rm, Matrix4& rootBone, Vector3& position, float& angle) {
	int bi = this->armature->rootBoneIdx;
	
	Matrix4 local = this->armature->localBones[bi];
	rootBone = Inverse(local) * armature->frames[idx][bi] * local;
	Vector4 posePosition = rootBone.getRow(3);
	Matrix4 worldRootBone = this->transform * Transpose(rootBone) * Inverse(this->transform);

	//Quat rotation = dmGameObject::GetRotation(this->root);

	if ((rm == RootMotion::Rotation) || (rm == RootMotion::Both)) {

		angle = QuatToEuler(Quat(worldRootBone.getUpper3x3())).getZ();
	
		Matrix4 mm = Matrix4::rotationZ(angle);
		mm =  worldRootBone * Inverse(mm);
		mm = Inverse(this->transform) * mm * this->transform;

		Matrix3 mXZ = mm.getUpper3x3();
		rootBone.setUpper3x3(Transpose(mXZ));

		mm = Matrix4::rotationZ(-angle);
		mm = Inverse(this->transform) * mm * this->transform;
		posePosition = mm * posePosition;

		rootBone.setRow(3, posePosition);
	} 
	
	if ((rm == RootMotion::Position) || (rm == RootMotion::Both)) {

		Vector4 v = this->transform * posePosition;
		position = Vector3(v.getX(), v.getZ(), -v.getY());

		/*
		position = dmVMath::Rotate(rotation, position);

		//------------remove Y ---------
		v = Inverse(this->transform) * Vector4(0, 0, position[1], 0);
		position[1] = 0;
		//------------------------------

		rootBone.setRow(3, v);
		*/
	}

	rootBone = local * rootBone * Inverse(local);
}

void Animation::ExtractRootMotion(RootMotion rm1, RootMotion rm2) {
	if (rm1 == RootMotion::None && rm2 == RootMotion::None) return;
	
	auto track = &this->tracks[0];
	int bi = this->armature->rootBoneIdx;
	
	Matrix4 rootBone1, rootBone2;
	Vector3 position1 = this->position1;
	Vector3 position2 = this->position2;
	float angle1 = this->angle1;
	float angle2 = this->angle2;

	bool applyRotation1 = (rm1 == RootMotion::Rotation) || (rm1 == RootMotion::Both);
	bool applyPosition1 = (rm1 == RootMotion::Position) || (rm1 == RootMotion::Both);
	bool applyRotation2 = (track->frame2 > -1) && ((rm2 == RootMotion::Rotation) || (rm2 == RootMotion::Both));
	bool applyPosition2 = (track->frame2 > -1) && ((rm2 == RootMotion::Position) || (rm2 == RootMotion::Both));


	Matrix4* bone1 = &this->armature->frames[track->frame1][bi];
	Matrix4* bone2 = track->frame2 > -1 ? &this->armature->frames[track->frame2][bi] : NULL;

	Quat r1 = Quat::identity();
	Quat r2 = Quat::identity();
	
	if (rm1 != RootMotion::None) {
		this->GetRootMotionForFrame(track->frame1, rm1, rootBone1, position1, angle1);
		bone1 = &rootBone1;

		r1 = Quat::rotationY(angle1 - this->angle1);
		this->angle1 = angle1;

		//dmLogInfo("setframe: %d, %f, %f, %f", track->frame1, position1[0],position1[1],position1[2]);
	}

	if (applyPosition2 || applyRotation2) {
		this->GetRootMotionForFrame(track->frame2, rm2, rootBone2, position2, angle2);
		bone2 = &rootBone2;
		
		r2 = Quat::rotationY(angle2 - this->angle2);
		this->angle2 = angle2;
	}

	Quat rotation = dmGameObject::GetRotation(this->root);

	if (applyRotation1 || applyRotation2) 
	{
		Quat diff = Slerp(track->factor, r1, r2);
		rotation = diff * rotation;
		dmGameObject::SetRotation(this->root, rotation);
	}


	Matrix4 local = this->armature->localBones[bi];
	
	if (applyPosition1) {
		position1 = dmVMath::Rotate(rotation, position1);

		//------------remove Y ---------
		Vector4 v = Inverse(this->transform) * Vector4(0, 0, position1[1], 0);
		position1[1] = 0;
		//------------------------------

		rootBone1 = Inverse(local) * rootBone1 * local;
		rootBone1.setRow(3, v);
		//rootBone1.setRow(3, Vector4(0));
		rootBone1 = local * rootBone1 * Inverse(local);
		
	}

	if (applyPosition2) {
		position2 = dmVMath::Rotate(rotation, position2);

		//------------remove Y ---------
		Vector4 v = Inverse(this->transform) * Vector4(0, 0, position2[1], 0);
		position2[1] = 0;
		//------------------------------

		
		rootBone2 = Inverse(local) * rootBone2 * local;
		rootBone2.setRow(3, v);
		//rootBone2.setRow(3, Vector4(0));
		rootBone2 = local * rootBone2 * Inverse(local);

	}

	/*
	if (applyPosition1) {
		Point3 p = dmGameObject::GetPosition(this->root);
		dmGameObject::SetPosition(this->root, p + position1 - this->position1);
		this->position1 = position1;
		
	}*/

	
	if (applyPosition1 || applyPosition2) 
	{
		Vector3 position = Slerp(track->factor, position1 - this->position1, position2 - this->position2);
		this->position1 = position1;
		this->position2 = position2;
		
		//Vector4 v1 = bone1->getRow(3);
		//Vector4 v2 = bone2 != NULL ? bone2->getRow(3) : Vector4(0);
		
		//dmLogInfo("%d, %d, position 1, %f, %f, %f", track->frame1, track->frame2, v1[0], v1[1], v1[2] );
		//dmLogInfo("%d, %d, position 2, %f, %f, %f", track->frame1, track->frame2, v2[0], v2[1], v2[2] );
		
		Point3 p = dmGameObject::GetPosition(this->root);
		dmGameObject::SetPosition(this->root, p + position);

	}

	
	if (track->frame2 == -1) {
		track->interpolated = armature->frames[track->frame1]; //copy
		track->bones = &track->interpolated;
		track->interpolated[bi] = *bone1;
	}else {
		MatrixBlend(bone1, bone2, &track->interpolated[bi], track->factor);
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
		this->CalculateBones(false, false);
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
			this->CalculateBones(false, false);

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
	this->CalculateBones(false, false); // return to first frame

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