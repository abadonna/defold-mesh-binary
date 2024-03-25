#include "reader.h"
#include <vector>

using namespace std;

class Model
{
	private:
		vector<Mesh>meshes;
		string* boneNames  = NULL;
		vector<SkinData>* skin = NULL;
		Vector4 inv_local_bones[3];
		
	
	public:
		string name;
		string parent;
		Transform3d local;
		Transform3d world;
		Vertex* vertices = NULL;
		vector<Vector4*> frames;
		unordered_map<string, unordered_map<uint32_t, ShapeData> > shapes;
		vector< unordered_map<string, float> > shapeFrames;

		void CreateLuaProxy(lua_State* L);
		
		Model(Reader* reader);
		~Model();
};
