#include "utils.h"
using namespace std;

class Mesh
{
	private:
		
	public:
		vector<Face> faces;
		vector<float>texcoords;
		Material material;
		
		void Test();
		Mesh();
		~Mesh();
};