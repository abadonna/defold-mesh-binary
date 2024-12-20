#include "mesh.h"
#include <iostream>

class Reader
{
	private:
		const char* data; 
		bool halfPrecision;
		
	public:
		bool IsEOF();
		int ReadInt();
		float ReadFloat();
		float ReadFloatHP();
		Vector3 ReadVector3();
		Vector4 ReadVector4();
		Matrix4 ReadMatrix();
		string ReadString();
		string ReadString(int size);
		Transform3d ReadTransform();
		Reader(const char* file);
		~Reader();
};