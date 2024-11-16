

#include "track.h"

namespace dmScript {
	dmMessage::URL* CheckURL(lua_State* L, int index);
	void PushURL(lua_State* L, dmMessage::URL const& url );
}

using namespace std;
using namespace dmMessage;

class Model;

class ModelInstance {
	private:
		vector<BoneGO> boneObjects;
		vector<Matrix4> cumulative;
		
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
		bool useBakedAnimations = false;
		unordered_map<int, ShapeData> blended;
		vector<dmScript::LuaHBuffer> buffers;
		vector<URL> urls;

		vector<AnimationTrack> tracks;

		vector<Matrix4>* bones = NULL;

		ModelInstance(Model* model, bool useBakedAnimations);
		~ModelInstance();
		
		void CreateLuaProxy(lua_State* L);
		void SetFrame(int trackIdx, int idx1, int idx2, float factor);
		void Update(lua_State* L);
		void CalculateBones();
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		
		URL* AttachGameObject(dmGameObject::HInstance go, string bone);

		Vertex* GetVertices();

		void AddAnimationTrack(vector<string>* mask);
};

class Instance{
	private:
		vector<ModelInstance*> models;

	public:
		void SetFrame(int trackIdx, int idx1, int idx2, float factor);
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		void CreateLuaProxy(lua_State* L);
		void Update(lua_State* L);
		URL* AttachGameObject(dmGameObject::HInstance go, string bone);

		int AddAnimationTrack(vector<string>* mask);
		void SetAnimationTrackWeight(int idx, float weight);
		
		Instance(vector<Model*>* data, bool useBakedAnimations);
		~Instance();
};
