#include "utils.h"
#include "armature.h"
#include "track.h"

using namespace std;

class Animation
{
	private:
		Armature* armature;
		vector<string> boneNames;
		vector<Matrix4> cumulative;
	
	public:
		vector<AnimationTrack> tracks;
		vector<Matrix4>* bones = NULL;

		void SetFrame(int trackIdx, int idx1, int idx2, float factor, bool useBakedAnimations, bool hasAttachaments);
		void Update(lua_State* L);
		void CalculateBones();
		void AddAnimationTrack(vector<string>* mask);
		void SetAnimationTrackWeight(int idx, float weight);
		void GetTextureBuffer(lua_State* L, Matrix4 local);
		int GetFramesCount();
		Vector4 GetBakedUniform();
		int FindBone(string bone);
		
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