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
	
		void CalculateTangents(Vertex* vertices);
		
	public:
		vector<Face> faces;
		vector<float> texcoords;
		set<int> usedBonesIndex;
		Material material;

		dmScript::LuaHBuffer CreateBuffer(ModelInstance* model);
		
		Mesh();
		~Mesh();
};