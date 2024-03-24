#include "reader.h"

Reader::Reader(const char* file, unsigned long fsize) {
	this->data = file;
	this->size = fsize;
}

Reader::~Reader() {
}

//-----------------------------------------------------------------

bool Reader::IsEOF() {
	return this->index >= this->size;
}

int Reader::ReadInt() {
	const unsigned char *b = (const unsigned char *)(this->data + this->index);
	int temp = ((b[3] << 24) |
	(b[2] << 16) |
	(b[1] <<  8) |
	b[0]);

	this->index += 4;
	return temp;
}

string Reader::ReadString(int size) {
	string res = string(&this->data[this->index], size);
	this->index += size;
	return res;
}

string Reader::ReadString() {
	int size = ReadInt();
	return ReadString(size);
}

//https://stackoverflow.com/questions/3991478/building-a-32-bit-float-out-of-its-4-composite-bytes
float Reader::ReadFloat() {
	const unsigned char *b = (const unsigned char *)(this->data + this->index);
	uint32_t temp = 0;
	temp = ((b[3] << 24) |
	(b[2] << 16) |
	(b[1] <<  8) |
	b[0]);

	this->index += 4;
	return *((float *) &temp);
}

Vector3 Reader::ReadVector3() {
	return Vector3(ReadFloat(), ReadFloat(), ReadFloat());
}

Vector4 Reader::ReadVector4() {
	return Vector4(ReadFloat(), ReadFloat(), ReadFloat(), ReadFloat());
}

Transform3d Reader::ReadTransform() {
	Transform3d res;
	Vector3 p = ReadVector3();
	res.position = Vector3(p.getX(), p.getZ(), -p.getY()); // blender coords fix

	Vector3 euler = ReadVector3();
	Quat qx = Quat::rotationX(euler.getX());
	Quat qy = Quat::rotationY(euler.getZ());
	Quat qz = Quat::rotationZ(-euler.getY());
	res.rotation = qy * qz * qx;

	Vector3 s = ReadVector3();
	res.scale = Vector3(s.getX(), s.getZ(), s.getY());

	Matrix4 mR = Matrix4::rotationX(euler.getX()) * Matrix4::rotationY(euler.getY()) * Matrix4::rotationZ(euler.getZ()); 
	Matrix4 mT = Matrix4::translation(p);
	Matrix4 mS = Matrix4();
	mS[0][0] = s.getX();
	mS[1][1] = s.getY();
	mS[2][2] = s.getZ();

	res.matrix =  Transpose(mT * mR * mS);
	
	return res;
}

	