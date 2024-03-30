#include "binary.h"

BinaryFile::BinaryFile(const char* file, unsigned long size) {
	Reader* reader = new Reader(file, size);

	while (!reader->IsEOF()) {
		Model* model = new Model(reader);
		this->models.push_back(model);

		//break;
	}

	delete reader;
}

BinaryFile::~BinaryFile() {
	for (auto & model : this->models) {
		delete model;
	}

	for (auto & instance : this->instances) {
		delete instance;
	}
}

Instance* BinaryFile::CreateInstance() {
	Instance* instance = new Instance(&this->models);
	this->instances.push_back(instance);
	return instance;
}