#include "model.h"

unsigned nextPOT(unsigned x) {
	if (x <= 1) return 2;
	int power = 2;
	x--;
	while (x >>= 1) power <<= 1;
	return power;
}

Model::Model(Reader* reader){
	this->meshes.emplace_back();
	//this->meshes.back().Test();

	this->name = reader->ReadString();
	dmLogInfo("reading: %s", this->name.c_str());

	int parentFlag = reader->ReadInt();

	if (parentFlag > 0) {
		this->parent = reader->ReadString(parentFlag);
	}

	this->local = reader->ReadTransform();
	this->world = reader->ReadTransform();

	this->vertexCount = reader->ReadInt();
	dmLogInfo("vertices: %d", this->vertexCount);

	this->vertices = new Vertex[vertexCount];
	for (int i = 0; i < vertexCount; i++) {
		this->vertices[i].p = reader->ReadVector3();
		this->vertices[i].n = reader->ReadVector3();
	}

	int shapeCount = reader->ReadInt();
	dmLogInfo("shapes: %d", shapeCount);

	for (int j = 0; j < shapeCount; j++) {
		string name = reader->ReadString();
		unordered_map<int, ShapeData> shape;
		
		int deltaCount = reader->ReadInt();
		
		for (int i = 0; i < deltaCount; i++) {
			int idx = reader->ReadInt();
			ShapeData data;
			data.p = reader->ReadVector3();
			data.n = reader->ReadVector3();
			//data.q = Quat::rotation(this->vertices[idx].n, this->vertices[idx].n + data.n);
			shape[idx] = data;
		}

		this->shapes[name] = shape;
	}
	
	int faceCount = reader->ReadInt();
	int faceMap[faceCount];
	
	dmLogInfo("faces: %d", faceCount);
	
	for (int i = 0; i < faceCount; i++) {
		Face face;
		face.v[0] = reader->ReadInt();
		face.v[1] = reader->ReadInt();
		face.v[2] = reader->ReadInt();

		int mi = reader->ReadInt();
		faceMap[i] = mi;
		
		int flatFlag = reader->ReadInt();
		if (flatFlag == 1) {
			face.isFlat = true;
			face.n = reader->ReadVector3();
		}

		while (this->meshes.size() <= mi) {
			this->meshes.emplace_back();
		}
		this->meshes[mi].faces.push_back(face);

		if (shapeCount > 0) {
			int idx = (this->meshes[mi].faces.size() - 1) * 3;
			this->meshes[mi].vertexMap[face.v[0]].push_back(idx);
			this->meshes[mi].vertexMap[face.v[1]].push_back(idx + 1);
			this->meshes[mi].vertexMap[face.v[2]].push_back(idx + 2);
		}
	}

	for (int i = 0; i < faceCount; i++) {
		Mesh* mesh = &this->meshes[faceMap[i]];
		for (int j = 0; j < 6; j++) {
			mesh->texcoords.push_back(reader->ReadFloat());
		}
	}

	int materialCount = reader->ReadInt();

	dmLogInfo("materials: %d", materialCount);

	Mesh notUsedMaterialMesh; //? still needed ?
	
	for (int i = 0; i < materialCount; i ++) {
		Mesh* mesh = (this->meshes.size() > i) ? &this->meshes[i] : &notUsedMaterialMesh;
		mesh->material.type = reader->ReadInt(); // 0 - opaque, 1 - blend, 2 - hashed
		mesh->material.color = reader->ReadVector4();
		mesh->material.specular.value = reader->ReadFloat();
		mesh->material.roughness.value = reader->ReadFloat();

		int textureFlag = reader->ReadInt();
		int rampFlag = 0;

		if (textureFlag > 0) {
			mesh->material.texture = reader->ReadString(textureFlag);
		}
		
		textureFlag = reader->ReadInt(); //normal texture
		if (textureFlag > 0) {
			mesh->material.normal.texture = reader->ReadString(textureFlag);
			mesh->material.normal.value = reader->ReadFloat();
		}

		textureFlag = reader->ReadInt(); //specular texture
		if (textureFlag > 0) {
			mesh->material.specular.texture = reader->ReadString(textureFlag);
			mesh->material.specular.invert = reader->ReadInt();
			rampFlag = reader->ReadInt();
			if (rampFlag > 0) {
				mesh->material.specular.ramp.p1 = reader->ReadFloat();
				mesh->material.specular.ramp.v1 = reader->ReadFloat();
				mesh->material.specular.ramp.p2 = reader->ReadFloat();
				mesh->material.specular.ramp.v2 = reader->ReadFloat();
			}
		}
	
		textureFlag = reader->ReadInt(); //roughness texture
		if (textureFlag > 0) {
			mesh->material.roughness.texture = reader->ReadString(textureFlag);
			rampFlag = reader->ReadInt();
			if (rampFlag > 0) {
				mesh->material.roughness.ramp.p1 = reader->ReadFloat();
				mesh->material.roughness.ramp.v1 = reader->ReadFloat();
				mesh->material.roughness.ramp.p2 = reader->ReadFloat();
				mesh->material.roughness.ramp.v2 = reader->ReadFloat();
			}
		}
	}

	//dmLogInfo("specular_value: %f", this->meshes[0].material.specular.value);
	

	int boneCount = reader->ReadInt();
	dmLogInfo("bones: %d", boneCount);
	
	if (boneCount == 0) {
		return;
	}

	//reading armature
	this->boneNames.reserve(boneCount);
	for (int i = 0; i < boneCount; i++) {
		this->boneNames.push_back(reader->ReadString());
	}

	
	this->skin = new vector<SkinData>[vertexCount];
	for (int i = 0; i < vertexCount; i++) {
		this->skin[i].reserve(4);
		int weightCount = reader->ReadInt();
		for (int j = 0; j < weightCount; j++) {
			SkinData data;
			data.idx = reader->ReadInt();
			data.weight = reader->ReadFloat();
			this->skin[i].push_back(data);
		}
	}

	
	this->isPrecomputed = (reader->ReadInt() == 1);
	
	if (!this->isPrecomputed) {
		this->invLocalBones.reserve(boneCount * 3);
		for (int i = 0; i < boneCount; i++) {
			//3x4 transform matrix
			this->invLocalBones.push_back(reader->ReadVector4());
			this->invLocalBones.push_back(reader->ReadVector4());
			this->invLocalBones.push_back(reader->ReadVector4());
			
		}
	}

	int frameCount = reader->ReadInt();
	dmLogInfo("frames: %d", frameCount);

	this->frames.reserve(frameCount);

	for (int i = 0; i < frameCount; i++) {
		vector<Vector4> bones;
		bones.reserve(boneCount * 3);
		for (int j = 0; j < boneCount; j++) {
			//3x4 transform matrix
			bones.push_back(reader->ReadVector4());
			bones.push_back(reader->ReadVector4());
			bones.push_back(reader->ReadVector4());
		}
		
		this->frames.push_back(bones);
		
		unordered_map<string, float> shapes;
		for (int j = 0; j < shapeCount; j++) {
			string key = reader->ReadString();
			float value = reader->ReadFloat();
			shapes[key] = value;
		}
		
		this->shapeFrames.push_back(shapes);
	}

	this->animationTextureWidth = nextPOT(this->frames[0].size());
	this->animationTextureHeight = nextPOT(frameCount);
	
}

Model::~Model(){
	delete [] this->vertices;
	delete [] this->skin;
}

int Model::FindBone(string bone) {
	for (int i = 0; i < this->boneNames.size(); i++) {
		if (bone == this->boneNames[i])
			return i;
	}

	return -1;
}
