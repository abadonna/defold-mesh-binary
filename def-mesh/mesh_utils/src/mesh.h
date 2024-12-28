#include "utils.h"
#include <set>

using namespace std;
using namespace dmMessage;

class ModelInstance;

class Mesh
{
	private:
		
	public:
		vector<Face> faces;
		vector<float> texcoords;
		set<int> usedBonesIndex;
		unordered_map< int, vector<int> > vertexMap;
		Material material;

		dmScript::LuaHBuffer CreateBuffer(ModelInstance* model, float scaleAABB);
		
		Mesh();
		~Mesh();
};