#include <dmsdk/sdk.h>
#include "utils.h"
#include <vector>

using namespace std;

class Model;
class Armature;

class AnimationTrack {
	private:
		vector<Matrix4> interpolated;

	public:
		int frame1 = -1;
		int frame2 = -1;
		float factor = 0.0;
		float weight = 1.0;
		bool additive = false; // blending mode: override, TODO: additive
		vector<int> mask;
		
		vector<Matrix4>* bones = NULL;

		void Interpolate(Armature* armature);
};