#include "binary.h"

BinaryFile::BinaryFile(const char* file, unsigned long size) {
	Reader* reader = new Reader(file, size);

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
}

Instance* BinaryFile::CreateInstance(bool useBakedAnimations) {
	Instance* instance = new Instance(&this->models, useBakedAnimations);
	this->instances ++;
	return instance;
}
