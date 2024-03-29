#include "utils.h"
using namespace std;

class ModelInstance;

class Mesh
{
	private:
		
	public:
		vector<Face> faces;
		vector<float>texcoords;
		Material material;
		
		dmBuffer::HBuffer CreateBuffer(ModelInstance* model);
		Mesh();
		~Mesh();
};