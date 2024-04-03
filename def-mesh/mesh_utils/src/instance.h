#include <dmsdk/sdk.h>
#include "utils.h"
#include <vector>

namespace dmScript {
	dmMessage::URL* CheckURL(lua_State* L, int index);
	void PushURL(lua_State* L, dmMessage::URL const& url );
}

using namespace std;
using namespace dmMessage;

class Model;

class ModelInstance {
	private:
		vector<Vector4> temp;
		vector<Vector4> interpolated;
		unordered_map<string, float> shapeValues;
		void Interpolate(int idx1, int idx2, float factor);
		void CalculateShapes(vector<string>* shapeNames);
		void ApplyShapes(lua_State* L);
		void ApplyArmature(lua_State* L, int meshIdx);
		void SetShapeFrame(lua_State* L, int idx1, int idx2, float factor);
	
	public:
		Model* model;
		bool useBakedAnimations = false;
		unordered_map<int, ShapeData> blended;
		vector<dmScript::LuaHBuffer> buffers;
		vector<URL> urls;
		
		int frame1 = -1;
		int frame2 = -1;
		float factor = 0;
		vector<Vector4>* bones = NULL;
		vector<Vector4>* calculated = NULL;

		ModelInstance(Model* model, bool baked);
		~ModelInstance();
		
		void CreateLuaProxy(lua_State* L);
		void SetFrame(lua_State* L, int idx1, int idx2, float factor);
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		void CalculateBones();

		Vertex* GetVertices();
};

class Instance{
	private:
		vector<ModelInstance*> models;

	public:
		void SetFrame(lua_State* L, int idx1, int idx2, float factor);
		void SetShapes(lua_State* L, unordered_map<string, float>* values);
		void CreateLuaProxy(lua_State* L);
		
		Instance(vector<Model*>* data, bool baked);
		~Instance();
};
