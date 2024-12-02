#include "utils.h"

using namespace std;

class BoneGameObject {
	private:
		int boneIdx;
		dmGameObject::HInstance gameObject;
	
	public:
		BoneGameObject(dmGameObject::HInstance obj, int idx);
		void ApplyTransform(vector<Matrix4>* bones);
};