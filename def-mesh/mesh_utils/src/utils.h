#pragma once

#include <dmsdk/sdk.h>
#include <unordered_map>
#include <string>
#include <vector>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

using namespace std;
using namespace dmVMath;

struct RootMotionData {
	float angle = 0;
	Vector3 position = Vector3(0);
	Vector4 offset = Vector4(0);
	Matrix4 rotation = Matrix4::identity();
};

struct Face {
	int v[3];
	bool isFlat = false;
	Vector3 n;
};

struct Vertex {
	Vector3 p;
	Vector3 n;
};

struct ShapeData : Vertex {
	//Quat q;
};


struct SkinData {
	int idx = -1;
	float weight = 0;
};

struct Transform3d {
	Vector3 position;
	Vector3 scale;
	Quat rotation;
	Matrix4 matrix;
};

struct Ramp {
	float p1 = 0;
	float v1 = 0;
	float p2 = 0;
	float v2 = 0;
};

struct NormalMap {
	string texture;
	float value = 0;
};

struct SpecularMap {
	string texture;
	float value = 0;
	int invert = 0;
	Ramp ramp;
};

struct RoughnessMap {
	string texture;
	float value = 0;
	Ramp ramp;
};

inline template <typename K, typename T>
bool CONTAINS(unordered_map<K, T>* map, K key){
	return map->find(key) != map->end();
}

Vector3 QuatToEuler(Quat q);

void MatrixBlend (Matrix4* m1, Matrix4* m2, Matrix4* result, float factor);

struct Material {
	string name;
	int type; // 0 - opaque, 1 - blend, 2 - hashed
	Vector4 color;
	string texture;
	NormalMap normal;
	SpecularMap specular;
	RoughnessMap roughness;
	
	void CreateLuaProxy(lua_State* L) {
		lua_pushstring(L, "material");
		lua_newtable(L);

		lua_pushstring(L, "name");
		lua_pushstring(L, this->name.c_str());
		lua_settable(L, -3);
		
		lua_pushstring(L, "type");
		lua_pushnumber(L, this->type);
		lua_settable(L, -3);

		lua_pushstring(L, "color");
		dmScript::PushVector4(L, this->color);
		lua_settable(L, -3);

		if (!this->texture.empty()) {
			lua_pushstring(L, "texture");
			lua_pushstring(L, this->texture.c_str());
			lua_settable(L, -3);
		}

		lua_pushstring(L, "normal");
		lua_newtable(L);
		lua_pushstring(L, "value");
		lua_pushnumber(L, this->normal.value);
		lua_settable(L, -3);
		if (!this->normal.texture.empty()) {
			lua_pushstring(L, "texture");
			lua_pushstring(L, this->normal.texture.c_str());
			lua_settable(L, -3);
		}
		lua_settable(L, -3);

		lua_pushstring(L, "specular");
		lua_newtable(L);
		lua_pushstring(L, "value");
		lua_pushnumber(L, this->specular.value);
		lua_settable(L, -3);
		lua_pushstring(L, "invert");
		lua_pushnumber(L, this->specular.invert);
		lua_settable(L, -3);
		if (!this->specular.texture.empty()) {
			lua_pushstring(L, "texture");
			lua_pushstring(L, this->specular.texture.c_str());
			lua_settable(L, -3);
		}
		lua_pushstring(L, "ramp");
		lua_newtable(L);
		lua_pushstring(L, "p1");
		lua_pushnumber(L, this->specular.ramp.p1);
		lua_settable(L, -3);
		lua_pushstring(L, "v1");
		lua_pushnumber(L, this->specular.ramp.v1);
		lua_settable(L, -3);
		lua_pushstring(L, "p2");
		lua_pushnumber(L, this->specular.ramp.p2);
		lua_settable(L, -3);
		lua_pushstring(L, "v2");
		lua_pushnumber(L, this->specular.ramp.v2);
		lua_settable(L, -3);
		lua_settable(L, -3);
		lua_settable(L, -3);

		lua_pushstring(L, "roughness");
		lua_newtable(L);
		lua_pushstring(L, "value");
		lua_pushnumber(L, this->roughness.value);
		lua_settable(L, -3);
		if (!this->roughness.texture.empty()) {
			lua_pushstring(L, "texture");
			lua_pushstring(L, this->roughness.texture.c_str());
			lua_settable(L, -3);
		}
		lua_pushstring(L, "ramp");
		lua_newtable(L);
		lua_pushstring(L, "p1");
		lua_pushnumber(L, this->roughness.ramp.p1);
		lua_settable(L, -3);
		lua_pushstring(L, "v1");
		lua_pushnumber(L, this->roughness.ramp.v1);
		lua_settable(L, -3);
		lua_pushstring(L, "p2");
		lua_pushnumber(L, this->roughness.ramp.p2);
		lua_settable(L, -3);
		lua_pushstring(L, "v2");
		lua_pushnumber(L, this->roughness.ramp.v2);
		lua_settable(L, -3);
		lua_settable(L, -3);
		lua_settable(L, -3);

		lua_settable(L, -3);
	}
};
