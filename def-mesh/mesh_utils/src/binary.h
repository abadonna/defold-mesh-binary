#include "model.h"

using namespace std;

class BinaryFile
{
	private:
		
	
	public:
		vector<Model> models;
	
		BinaryFile() = default;
		BinaryFile(const char* file, unsigned long size);
		~BinaryFile();
};