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
	
	public:
		vector<Matrix4>* bones = NULL;

		void SetFrame(int trackIdx, int idx1, int idx2, float factor, bool useBakedAnimations, bool hasAttachaments);
		void Update();
		void CalculateBones();
		int AddAnimationTrack(vector<string>* mask);
		void SetTrackWeight(int idx, float weight);
		int GetTextureBuffer(lua_State* L);
		int GetFramesCount();
		Vector4 GetBakedUniform();
		int FindBone(string bone);
		int GetFrameIdx();
		
		Animation(Armature* armature);
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
			Quat q = MatToQuat(m);
			dmGameObject::SetRotation(this->gameObject, Quat(q.getX(), q.getZ(), -q.getY(), q.getW()));
			dmGameObject::SetPosition(this->gameObject, dmVMath::Point3(v1.getW(), v3.getW(), -v2.getW()));
		}
};