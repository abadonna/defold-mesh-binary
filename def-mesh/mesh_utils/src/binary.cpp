#include "binary.h"

BinaryFile::BinaryFile(const char* file, bool verbose) {
	Reader* reader = new Reader(file);

	int count = reader->ReadInt();
	this->armatures.reserve(count);
	for (int i = 0; i < count; i++) {
		Armature* armature = new Armature(reader, verbose);
		this->armatures.push_back(armature);
	}

	while (!reader->IsEOF()) {
		Model* model = new Model(reader, verbose);
		this->models.push_back(model);
	}

	delete reader;
}

BinaryFile::~BinaryFile() {
	for (auto & model : this->models) {
		delete model;
	}

	for (auto & armature : this->armatures) {
		delete armature;
	}
}

Instance* BinaryFile::CreateInstance(dmGameObject::HInstance obj, bool useBakedAnimations, float scaleAABB) {
	Instance* instance = new Instance(&this->models, &this->armatures, obj, useBakedAnimations, scaleAABB);
	this->instances ++;
	return instance;
}
