#include "binary.h"

BinaryFile::BinaryFile(const char* file, unsigned long size) {
	Reader* reader = new Reader(file, size);

	while (!reader->IsEOF()) {
		this->models.emplace_back(reader);

		//break;
	}

	delete reader;
}

BinaryFile::~BinaryFile() {
}