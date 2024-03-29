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
}

Instance* BinaryFile::CreateInstance() {
	this->instances.emplace_back(&this->models);
	return &this->instances.back();
}