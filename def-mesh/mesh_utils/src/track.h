#include <dmsdk/sdk.h>
#include "utils.h"
#include <vector>

using namespace std;

class Model;
class Armature;

enum class RootMotionType { None, Rotation, Position, Both, Forward };

class AnimationTrack {
	private:
		Armature* armature;
		Matrix4 transform;
		RootMotionData rmdata1;
		RootMotionData rmdata2;
		void GetRootMotionForFrame(int idx, RootMotionData* data, RootMotionType rm, Matrix4& rootBone, Vector3& position, float& angle);
		
	public:

		vector<Matrix4> interpolated;
	
		int frame1 = -1;
		int frame2 = -1;
		float factor = 0.0;
		float weight = 1.0;
		bool additive = false; // blending mode: override, TODO: additive
		vector<int> mask;
		
		vector<Matrix4>* bones = NULL;

		void Interpolate();
		void ExtractRootMotion(dmGameObject::HInstance root, RootMotionType rm1, RootMotionType rm2);
		void ResetRootMotion(int frameIdx, bool isPrimary);
		void SetTransform(Matrix4 matrix);
		void SwitchRootMotion();
		
		AnimationTrack(Armature* armature, vector<string>* bones);
};