#include "reader.h"
#include <vector>

using namespace std;

class Model
{
	private:
		string* boneNames  = NULL;
		
	
	public:
		string name;
		string parent;
		Transform3d local;
		Transform3d world;
		Vertex* vertices = NULL;
		vector<Mesh>meshes;
		vector< vector<Vector4> > frames;
		unordered_map<string, unordered_map<uint32_t, ShapeData> > shapes;
		vector< unordered_map<string, float> > shapeFrames;
		vector<SkinData>* skin = NULL;
		vector<Vector4> invLocalBones;
		bool isPrecomputed = false;
		
		Model(Reader* reader);
		~Model();
};
