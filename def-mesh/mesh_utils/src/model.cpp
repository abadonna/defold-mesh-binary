#include "model.h"

Model::Model(const char* file, unsigned long size) {
	Reader* reader = new Reader(file, size);

	while (!reader->IsEOF()) {
		Entity* entity = new Entity(reader);
		this->entities.push_back(entity);

		//break;
	}

	delete reader;
}

Model::~Model() {
	for (auto it = this->entities.begin(); it !=  this->entities.end(); it++) {
		delete *it;
	}
}