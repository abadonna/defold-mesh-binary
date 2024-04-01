#include "utils.h"
#include <set>

using namespace std;
using namespace dmMessage;

class ModelInstance;

class Mesh
{
	private:
		vector<float> tangents;
		vector<float> bitangents;
		set<int> usedBonesIndex;
	
		void CalculateTangents(Vertex* vertices);
		
	public:
		vector<Face> faces;
		vector<float> texcoords;
		Material material;

		dmBuffer::HBuffer CreateBuffer(ModelInstance* model);
		void ApplyArmature(lua_State* L, ModelInstance* mi, URL* url);
		
		
		Mesh();
		~Mesh();
};