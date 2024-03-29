#include "model.h"
#include "instance.h"

using namespace std;

class BinaryFile
{
	private:
		vector<Instance> instances;
	
	public:
		vector<Model*> models;
		

		Instance* CreateInstance();
	
		BinaryFile() = default;
		BinaryFile(const char* file, unsigned long size);
		~BinaryFile();
};