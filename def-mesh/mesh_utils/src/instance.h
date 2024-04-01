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
	
	public:
		Model* model;
		bool useBakedAnimations = false;
		Vertex* blended = NULL;
		vector<dmBuffer::HBuffer> buffers;
		vector<URL> urls;
		
		int frame1 = -1;
		int frame2 = -1;
		float factor = 0;
		vector<Vector4>* bones = NULL;
		vector<Vector4>* calculated = NULL;
		vector<Vector4> temp;

		ModelInstance(Model* model, bool baked);
		~ModelInstance();
		
		void CreateLuaProxy(lua_State* L);
		void SetFrame(lua_State* L, int idx1, int idx2, float factor);
		void CalculateBones();

		Vertex* GetVertices();
};

class Instance{
	private:
		vector<ModelInstance*> models;

	public:
		void SetFrame(lua_State* L, int frame);
		void CreateLuaProxy(lua_State* L);
		
		Instance(vector<Model*>* data, bool baked);
		~Instance();
};
