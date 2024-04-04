#include "mesh.h"
#include <iostream>

class Reader
{
	private:
		const char* data; 

	public:
		bool IsEOF();
		int ReadInt();
		float ReadFloat();
		Vector3 ReadVector3();
		Vector4 ReadVector4();
		string ReadString();
		string ReadString(int size);
		Transform3d ReadTransform();
		Reader(const char* file);
		~Reader();
};