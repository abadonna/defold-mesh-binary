#include <dmsdk/sdk.h>
#include "utils.h"
#include <vector>

using namespace std;

class Model;

class ModelInstance {
	private:
		Model* model;
	public:
		Vertex* blended = NULL;
		vector<dmBuffer::HBuffer> buffers;

		Vertex* GetVertices();
		ModelInstance(Model* model);
		~ModelInstance();
		void CreateLuaProxy(lua_State* L);
};

class Instance{
	private:
		vector<ModelInstance*> models;

	public:
		void CreateLuaProxy(lua_State* L);
		Instance(vector<Model*>* data);
		~Instance();
};

static int InstanceTest(lua_State* L) {
	//lua_getfield(L, -1, "idx");
	//int idx = luaL_checknumber(L, -1);
	//dmLogInfo("input index: %d", idx);

	lua_pushstring(L, "xxx");
	return 1;
}