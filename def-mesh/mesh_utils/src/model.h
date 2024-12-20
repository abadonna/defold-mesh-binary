#include "reader.h"
#include <vector>

using namespace std;

class Model
{
	private:
	
	public:
		string name;
		string parent;
		Transform3d world;
		int vertexCount = 0;
		int armatureIdx = 0;
		Vertex* vertices = NULL;
		vector<Mesh>meshes;

		unordered_map<string, unordered_map<int, ShapeData> > shapes;
		vector< unordered_map<string, float> > shapeFrames;
		vector<SkinData>* skin = NULL;
		
		Model(Reader* reader, bool verbose);
		~Model();

		vector<int> boneParents;
};
