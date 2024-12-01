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
		void SetFrame(int trackIdx, int idx1, int idx2, float factor, RootMotionType rm1, RootMotionType rm2);
		void Update(lua_State* L);
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		
		URL* AttachGameObject(BoneGO* obj, string bone);
};