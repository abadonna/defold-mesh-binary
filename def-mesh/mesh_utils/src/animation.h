#include "utils.h"
#include "armature.h"
#include "track.h"
#include "bonego.h"

using namespace std;

enum class RootMotionType { None, Rotation, Position, Both };

class Animation
{
	private:
		bool needUpdate = true;
		Armature* armature;
		vector<string> boneNames;
		vector<Matrix4> cumulative;
		vector<AnimationTrack> tracks;

		Matrix4 transform;
		dmGameObject::HInstance root = 0;

		vector<BoneGameObject> boneObjects;

		void CalculateBones(bool applyRotation, bool applyPosition);
		void ExtractRootMotion(RootMotionType rm1, RootMotionType rm2);
		void GetRootMotionForFrame(int idx, RootMotionType rm, Matrix4& rootBone, Vector3& position, float& angle);
	
	public:
		vector<Matrix4>* bones = NULL;

		RootMotionData rmdata1;
		RootMotionData rmdata2;
		
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
		void SetTransform(Matrix4* matrix);
		void ResetRootMotion(int frameIdx, bool isPrimary);
		void CreateBoneGO(dmGameObject::HInstance go, int idx);
		
		Animation(Armature* armature, dmGameObject::HInstance obj);
};
