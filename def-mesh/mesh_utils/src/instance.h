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

		ModelInstance(Model* model);
		~ModelInstance();
		
		void CreateLuaProxy(lua_State* L);
		void SetFrame(int frame);

		Vertex* GetVertices();
};

class Instance{
	private:
		vector<ModelInstance*> models;

	public:
		void SetFrame(int frame);
		void CreateLuaProxy(lua_State* L);
		
		Instance(vector<Model*>* data);
		~Instance();
};
