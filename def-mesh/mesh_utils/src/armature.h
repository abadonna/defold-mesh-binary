#include "utils.h"

class Reader;

using namespace std;

class Armature
{
	private:
		vector<string> boneNames;
	
	public:
		vector< vector<Matrix4> > frames;
		vector<Matrix4> localBones;
		vector<int> boneParents;

		int rootBoneIdx = 0;

		int animationTextureWidth = 0;
		int animationTextureHeight = 0;
		
		int FindBone(string bone);
		int GetFramesCount();
		
		Armature(Reader* reader, bool verbose);
		~Armature();
};
