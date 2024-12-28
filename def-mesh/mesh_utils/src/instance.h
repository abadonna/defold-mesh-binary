#include "model_instance.h"

using namespace std;
using namespace dmMessage;

class Model;

class Instance{
	private:
		vector<ModelInstance*> models;
		vector<Animation*> animations;
		bool useBakedAnimations = false;

	public:
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		void CreateLuaProxy(lua_State* L);
		void SetFrame(int trackIdx, int idx1, int idx2, float factor, RootMotionType rm1, RootMotionType rm2);
		void Update(lua_State* L);
		URL* AttachGameObject(dmGameObject::HInstance go, string bone);
		int AddAnimationTrack(vector<string>* mask);
		void SetAnimationTrackWeight(int idx, float weight);

		void ResetRootMotion(bool isPrimary, int frame);
		void SwitchRootMotion();
		
		Instance(vector<Model*>* models, vector<Armature*>* armatures, dmGameObject::HInstance obj, bool useBakedAnimations, float scaleAABB);
		~Instance();
};
