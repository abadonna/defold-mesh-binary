#include "animation.h"

namespace dmScript {
	dmMessage::URL* CheckURL(lua_State* L, int index);
	void PushURL(lua_State* L, dmMessage::URL const& url );
}

using namespace std;
using namespace dmMessage;

class Model;

class ModelInstance {
	private:
		unordered_map<string, float> shapeValues;
		unordered_map<string, float> directShapeValues;
		float threshold = 0;
		
		void CalculateShapes(vector<string>* shapeNames);
		void ApplyShapes(lua_State* L);
		void ApplyArmature(lua_State* L, int meshIdx);
		void ApplyTransform(BoneGO* obj);
		void SetShapeFrame(lua_State* L, int idx1);
	
	public:
		Model* model;
		Animation* animation = NULL;
		bool useBakedAnimations = false;
		unordered_map<int, ShapeData> blended;
		vector<dmScript::LuaHBuffer> buffers;
		vector<URL> urls;
		dmhash_t runtimeTexture;

		ModelInstance(Model* model, Animation* animation, bool useBakedAnimations);
		~ModelInstance();
		
		void CreateLuaProxy(lua_State* L);
		void SetFrame(int trackIdx, int idx1, int idx2, float factor);
		void Update(lua_State* L);
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		
		URL* AttachGameObject(BoneGO* obj, string bone);
};

class Instance{
	private:
		vector<BoneGO> boneObjects;
		vector<ModelInstance*> models;
		vector<Animation*> animations;
		bool useBakedAnimations = false;

	public:
		dmGameObject::HInstance root = 0;
		bool rootTransformPosition = false;
		bool rootTransformRotation = false;
	
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		void CreateLuaProxy(lua_State* L);
		void SetFrame(int trackIdx, int idx1, int idx2, float factor);
		void Update(lua_State* L);
		URL* AttachGameObject(dmGameObject::HInstance go, string bone);
		int AddAnimationTrack(vector<string>* mask);
		void SetAnimationTrackWeight(int idx, float weight);

		void ResetRootTransform(int frame);
		
		Instance(vector<Model*>* models, vector<Armature*>* armatures, bool useBakedAnimations);
		~Instance();
};
