#include "utils.h"
using namespace std;

class ModelInstance;

class Mesh
{
	private:
		vector<float> tangents;
		vector<float> bitangents;
	
		void CalcTangents(Vertex* vertices);
		
	public:
		vector<Face> faces;
		vector<float> texcoords;
		Material material;

		dmBuffer::HBuffer CreateBuffer(ModelInstance* model);
		void SetFrame(ModelInstance* model, int frame);
		
		Mesh();
		~Mesh();
};