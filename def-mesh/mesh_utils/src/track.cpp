#include "track.h"
#include "model.h"
#include "armature.h"

AnimationTrack::AnimationTrack(Armature* armature, vector<string>* bones) {
	this->armature = armature;
	this->transform = Matrix4::identity();
	this->inversed = Matrix4::identity();

	if (bones != NULL) {
		for (auto & bone : *bones) {
			int idx = armature->FindBone(bone);
			if (idx > -1) {
				this->mask.push_back(idx);
			}
		}
	}
}

void AnimationTrack::Interpolate() {
	int size = this->armature->frames[this->frame1].size();

	if (this->interpolated.size() != size) {
		this->interpolated = this->armature->frames[this->frame1];
	}

	for (int i = 0; i < size; i ++) { //TODO: optimize, interpolate using track mask
		MatrixBlend(
			&this->armature->frames[this->frame1].at(i), 
			&this->armature->frames[this->frame2].at(i), 
			&this->interpolated[i], 
			this->factor);
	}

	this->bones = &interpolated;
}

void AnimationTrack::SwitchRootMotion() {
	std::swap(this->rmdata1, this->rmdata2);
}

void AnimationTrack::ResetRootMotion(int frameIdx, bool isPrimary) {
	RootMotionData* data = isPrimary ? &this->rmdata1 : &this->rmdata2;

	int bi = this->armature->rootBoneIdx;
	Matrix4 local = this->armature->localBones[bi];

	Matrix4 m = this->armature->frames[frameIdx][bi];
	m = local * m * Inverse(local);
	data->rotation = m;

	Vector4 v = this->transform * m.getCol3();
	data->position = Vector3(v[0], v[2], -v[1]);

	data->offset = m.getCol3();

	//m = this->transform * m * Inverse(this->transform);
	data->angle = 0; // QuatToEuler(Quat(m.getUpper3x3())).getZ();	
}

void AnimationTrack::SetTransform(Matrix4 matrix) {
	this->transform = matrix;
	this->inversed = Inverse(this->transform);
	
	this->ResetRootMotion(0, true);
	this->ResetRootMotion(0, false);
}


//TODO: refactoring
void AnimationTrack::GetRootMotionForFrame(int idx, RootMotionData* data, RootMotionType rm, Matrix4& rootBone, Vector3& position, float& angle) {
	int bi = this->armature->rootBoneIdx;

	Matrix4 local = this->armature->localBones[bi];
	rootBone = local * armature->frames[idx][bi] * Inverse(local);

	Vector4 posePosition = rootBone.getCol3();
	Matrix4 worldRootBone = this->transform * rootBone * Inverse(data->rotation) * this->inversed;

	if ((rm == RootMotionType::Rotation) || (rm == RootMotionType::Both)) {

		angle = QuatToEuler(Quat(worldRootBone.getUpper3x3())).getZ();

		Matrix4 inv = Matrix4::rotationZ(-angle);
		Matrix4 mm =  worldRootBone * inv;
		mm = this->inversed * mm * this->transform;

		mm = mm * (data->rotation);

		Matrix3 mXZ = mm.getUpper3x3();
		rootBone.setUpper3x3(mXZ);

		mm = this->inversed * inv * this->transform;

		Vector4 pPosition = mm * posePosition;

		rootBone.setCol3(pPosition);
	} 

	if ((rm == RootMotionType::Position) || (rm == RootMotionType::Both) || (rm == RootMotionType::Forward)) {

		Vector4 v = this->transform * posePosition;
		position = Vector3(v.getX(), v.getZ(), -v.getY());

		if (rm == RootMotionType::Forward) { // bake Y & X into pose, move game object only along Z
			v = this->inversed * Vector4(v[0], 0, v[2], 1) + Vector4(0, data->offset[1], 0, 0);
			Vector4 t = this->transform * data->offset;
			position[0] = t[0];
			position[1] = t[2];
		} else {
			v = data->offset;
		}
		
		rootBone.setCol3(v);
	}

	rootBone = Inverse(local) * rootBone * local;
}

void AnimationTrack::ExtractRootMotion(dmGameObject::HInstance root, RootMotionType rm1, RootMotionType rm2) {
	if (rm1 == RootMotionType::None && rm2 == RootMotionType::None) return;

	int bi = this->armature->rootBoneIdx;

	Matrix4 rootBone1, rootBone2;
	Vector3 position1 = Vector3(0);
	Vector3 position2 = Vector3(0);
	float angle1 = this->rmdata1.angle;
	float angle2 = this->rmdata2.angle;

	bool applyRotation1 = (rm1 == RootMotionType::Rotation) || (rm1 == RootMotionType::Both);
	bool applyPosition1 = (rm1 == RootMotionType::Position) || (rm1 == RootMotionType::Both) || (rm1 == RootMotionType::Forward);
	bool applyRotation2 = (this->frame2 > -1) && ((rm2 == RootMotionType::Rotation) || (rm2 == RootMotionType::Both));
	bool applyPosition2 = (this->frame2 > -1) && ((rm2 == RootMotionType::Position) || (rm2 == RootMotionType::Both) || (rm2 == RootMotionType::Forward));


	Matrix4* bone1 = &this->armature->frames[this->frame1][bi];
	Matrix4* bone2 = this->frame2 > -1 ? &this->armature->frames[this->frame2][bi] : NULL;

	Quat r1 = Quat::identity();
	Quat r2 = Quat::identity();

	if (rm1 != RootMotionType::None) {
		this->GetRootMotionForFrame(this->frame1, &this->rmdata1, rm1, rootBone1, position1, angle1);
		bone1 = &rootBone1;

		r1 = Quat::rotationY(angle1 - this->rmdata1.angle);
		this->rmdata1.angle = angle1;

	}

	if (applyPosition2 || applyRotation2) {
		this->GetRootMotionForFrame(this->frame2, &this->rmdata2, rm2, rootBone2, position2, angle2);
		bone2 = &rootBone2;

		r2 = Quat::rotationY(angle2 - this->rmdata2.angle);
		this->rmdata2.angle = angle2;
	}

	Quat rotation = dmGameObject::GetRotation(root);

	if (applyRotation1 || applyRotation2) 
	{
		Quat diff = Slerp(this->factor, r1, r2);
		rotation = diff * rotation;
		dmGameObject::SetRotation(root, rotation);
	}


	Matrix4 local = this->armature->localBones[bi];

	if (applyPosition1) {

		Vector3 velocity = position1 - this->rmdata1.position;
		this->rmdata1.position = position1;

		if (applyRotation1) {
			Matrix3 mm = Matrix3::rotationY(-angle1);
			velocity = mm * velocity;
		}

		position1 = dmVMath::Rotate(rotation, velocity);	
		//dmLogInfo("%d, %f, %f, %f", this->GetFrameIdx(), position1[0], position1[1], position1[2]);

	}


	if (applyPosition2) {
		Vector3 velocity = position2 - this->rmdata2.position;
		this->rmdata2.position = position2;

		if (applyRotation2) {
			Matrix3 mm = Matrix3::rotationY(-angle2);
			velocity = mm * velocity;
		}

		position2 = dmVMath::Rotate(rotation, velocity);
	}


	if (applyPosition1 || applyPosition2) 
	{
		Point3 p = dmGameObject::GetPosition(root);
		Vector3 position = Slerp(this->factor, position1, position2);

		dmGameObject::SetPosition(root, p + position);
	}

	if (this->frame2 == -1) {
		this->interpolated = armature->frames[this->frame1]; //copy
		this->bones = &this->interpolated;
		this->interpolated[bi] = *bone1;
		Vector4 v = bone1->getRow(3);

	}else {
		MatrixBlend(bone1, bone2, &this->interpolated[bi], this->factor);
	}

}