#include "reader.h"

Reader::Reader(const char* file) {
	this->data = file;
}

Reader::~Reader() {
}

//-----------------------------------------------------------------

bool Reader::IsEOF() {
	return *this->data == '\0';
}

int Reader::ReadInt() {
	const unsigned char *b = (const unsigned char *)this->data;
	int value = b[0] | ((int)b[1] << 8) | ((int)b[2] << 16) | ((int)b[3] << 24);

	this->data += 4;
	return value;
}

string Reader::ReadString(int size) {
	string res = string(this->data, size);
	this->data += size;
	return res;
}

string Reader::ReadString() {
	int size = ReadInt();
	return ReadString(size);
}

//https://stackoverflow.com/questions/3991478/building-a-32-bit-float-out-of-its-4-composite-bytes
float Reader::ReadFloat() {
	const unsigned char *b = (const unsigned char *)this->data;
	uint32_t temp = 0;

	temp = ((b[3] << 24) |
	(b[2] << 16) |
	(b[1] <<  8) |
	b[0]);

	this->data += 4;
	return *((float *) &temp);
}

Vector3 Reader::ReadVector3() {
	float x = ReadFloat();
	float y = ReadFloat();
	float z = ReadFloat();
	return Vector3(x, y, z);
}

Vector4 Reader::ReadVector4() {
	float x = ReadFloat();
	float y = ReadFloat();
	float z = ReadFloat();
	float w = ReadFloat();
	return Vector4(x, y, z, w);
}

Matrix4 Reader::ReadMatrix() { //3x4
	Matrix4 m = Matrix4::identity();
	m.setRow(0, this->ReadVector4());
	m.setRow(1, this->ReadVector4());
	m.setRow(2, this->ReadVector4());
	return m;
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

	Matrix4 mR = Matrix4::rotationZYX(euler);
	Matrix4 mT = Matrix4::translation(p);
	Matrix4 mS = Matrix4::identity();
	mS[0][0] = s.getX();
	mS[1][1] = s.getY();
	mS[2][2] = s.getZ();

	//res.matrix = Transpose(mT * mR * mS);
	res.matrix = mT * mR * mS;
	
	return res;
}

	