#include "entity.h"

using namespace std;

class Model
{
	private:
		vector<Entity*> entities;
	
	public:
		Model(const char* file, unsigned long size);
		~Model();
};