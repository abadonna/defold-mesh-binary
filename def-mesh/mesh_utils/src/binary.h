#include "model.h"
#include "instance.h"

using namespace std;

class BinaryFile
{
	private:
		vector<Model*> models;
	public:
		int instances = 0;

		Instance* CreateInstance(bool useBakedAnimations);
	
		BinaryFile(const char* file);
		~BinaryFile();
};