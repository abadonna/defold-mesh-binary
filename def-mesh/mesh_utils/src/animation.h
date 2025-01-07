#include "utils.h"
#include "armature.h"
#include "track.h"
#include "bonego.h"

using namespace std;

class Animation
{
	private:
		bool needUpdate = true;
		bool hasRootMotion = false;
		Armature* armature;
		vector<string> boneNames;
		vector<Matrix4> cumulative;
		vector<AnimationTrack> tracks;

		Matrix4 transform;
		dmGameObject::HInstance root = 0;

		vector<BoneGameObject> boneObjects;

		void CalculateBones();

	public:
		vector<Matrix4>* bones = NULL;

		void SetFrame(int trackIdx, int idx1, int idx2, float factor, RootMotionType rm1, RootMotionType rm2);
		void Update();
		int AddAnimationTrack(vector<string>* mask);
		void SetTrackWeight(int idx, float weight);
		int GetTextureBuffer(lua_State* L);
		int GetRuntimeBuffer(lua_State* L);
		int GetFramesCount();
		Vector4 GetBakedUniform();
		int FindBone(string bone);
		int GetFrameIdx();
		bool IsBlending();
		bool HasAttachments();
		bool HasRootMotion();
		void SetTransform(Matrix4 matrix);
		void SwitchRootMotion();
		void ResetRootMotion(int frameIdx, bool isPrimary);
		void CreateBoneGO(dmGameObject::HInstance go, int idx);

		Animation(Armature* armature, dmGameObject::HInstance obj);
};
