#include "utils.h"
#include "armature.h"
#include "track.h"

using namespace std;

class Animation
{
	private:
		bool needUpdate = true;
		Armature* armature;
		vector<string> boneNames;
		vector<Matrix4> cumulative;
		vector<AnimationTrack> tracks;

		Matrix4 transform;
		float angle;
		Vector3 position;
		
		dmGameObject::HInstance root = 0;

		void CalculateBones(bool applyRotation, bool applyPosition);
	
	public:
		vector<Matrix4>* bones = NULL;
		
		void SetFrame(int trackIdx, int idx1, int idx2, float factor, bool useBakedAnimations, bool hasAttachaments);
		void Update(bool rotation, bool position);
		int AddAnimationTrack(vector<string>* mask);
		void SetTrackWeight(int idx, float weight);
		int GetTextureBuffer(lua_State* L);
		int GetRuntimeBuffer(lua_State* L);
		int GetFramesCount();
		Vector4 GetBakedUniform();
		int FindBone(string bone);
		int GetFrameIdx();
		bool IsBlending();
		void SetTransform(Matrix4* matrix, int frame);
		
		Animation(Armature* armature, dmGameObject::HInstance obj);
};

class BoneGO {
	public:
	
		int boneIdx;
		Animation* animation;
		dmGameObject::HInstance gameObject;
		void ApplyTransform() {

			Matrix4 m = this->animation->bones->at(this->boneIdx);
			m.setCol3(Vector4(1));

			Vector4 v1 = m.getCol0();
			Vector4 v2 = m.getCol1();
			Vector4 v3 = m.getCol2();

			m = Transpose(m);
			Quat q = Quat(m.getUpper3x3());
			dmGameObject::SetRotation(this->gameObject, Quat(q.getX(), q.getZ(), -q.getY(), q.getW()));
			dmGameObject::SetPosition(this->gameObject, dmVMath::Point3(v1.getW(), v3.getW(), -v2.getW()));
		}
};