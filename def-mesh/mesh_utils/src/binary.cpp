#include "binary.h"

BinaryFile::BinaryFile(const char* file) {
	Reader* reader = new Reader(file);

	int count = reader->ReadInt();
	this->armatures.reserve(count);
	for (int i = 0; i < count; i++) {
		Armature* armature = new Armature(reader);
		this->armatures.push_back(armature);
	}

	while (!reader->IsEOF()) {
		Model* model = new Model(reader);
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

Instance* BinaryFile::CreateInstance(bool useBakedAnimations) {
	Instance* instance = new Instance(&this->models, &this->armatures, useBakedAnimations);
	this->instances ++;
	return instance;
}
