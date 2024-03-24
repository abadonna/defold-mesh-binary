#include <dmsdk/sdk.h>
#include <unordered_map>
#include <string>

using namespace std;
using namespace dmVMath;

struct Face {
	int v[3];
	Vector3 n;
};

struct Vertex {
	Vector3 p;
	Vector3 n;
};

struct ShapeData {
	Vector3 p;
	Vector3 n;
	Quat q;
};


struct SkinData {
	int idx;
	float weight;
};

struct Transform3d {
	Vector3 position;
	Vector3 scale;
	Quat rotation;
	Matrix4 matrix;
};

struct Ramp {
	float p1;
	float v1;
	float p2;
	float v2;
};

struct NormalMap {
	string texture;
	float value;
};

struct SpecularMap {
	string texture;
	float value;
	int invert;
	Ramp ramp;
};

struct RoughnessMap {
	string texture;
	float value;
	Ramp ramp;
};

struct Material {
	int type; // 0 - opaque, 1 - blend, 2 - hashed
	Vector4 color;
	string texture;
	NormalMap normal;
	SpecularMap specular;
	RoughnessMap roughness;
};