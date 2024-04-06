#include "reader.h"
#include <vector>

using namespace std;

class Model
{
	private:
		vector<string> boneNames;
		
	
	public:
		string name;
		string parent;
		Transform3d local;
		Transform3d world;
		int vertexCount = 0;
		Vertex* vertices = NULL;
		vector<Mesh>meshes;
		vector< vector<Vector4> > frames;
		unordered_map<string, unordered_map<int, ShapeData> > shapes;
		vector< unordered_map<string, float> > shapeFrames;
		vector<SkinData>* skin = NULL;
		vector<Vector4> invLocalBones;
		bool isPrecomputed = false;
		int animationTextureWidth = 0;
		int animationTextureHeight = 0;

		int FindBone(string bone);
		
		Model(Reader* reader);
		~Model();
};
