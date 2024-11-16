#include "reader.h"
#include <vector>

using namespace std;

class Model
{
	private:
		vector<string> boneNames; //TODO: refactor to bone class or even better move to Armature
	
	public:
		string name;
		string parent;
		Transform3d local;
		Transform3d world;
		int vertexCount = 0;
		Vertex* vertices = NULL;
		vector<Mesh>meshes;
		vector< vector<Matrix4> > frames;
		unordered_map<string, unordered_map<int, ShapeData> > shapes;
		vector< unordered_map<string, float> > shapeFrames;
		vector<SkinData>* skin = NULL;
		vector<Matrix4> localBones;
		bool isPrecomputed = false;
		int animationTextureWidth = 0;
		int animationTextureHeight = 0;

		int FindBone(string bone);
		
		Model(Reader* reader);
		~Model();

		vector<int> boneParents;
};
