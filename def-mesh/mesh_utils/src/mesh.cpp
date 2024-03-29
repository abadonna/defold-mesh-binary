#include "mesh.h"
#include "instance.h"
#include <dmsdk/sdk.h>

Mesh::Mesh(){
}

Mesh::~Mesh(){
}

dmBuffer::HBuffer Mesh::CreateBuffer(ModelInstance* model) {
	const dmBuffer::StreamDeclaration streams_decl[] = {
		{dmHashString64("position"), dmBuffer::VALUE_TYPE_FLOAT32, 3},
		{dmHashString64("normal"), dmBuffer::VALUE_TYPE_FLOAT32, 3},
		{dmHashString64("texcoord0"), dmBuffer::VALUE_TYPE_FLOAT32, 2},
		{dmHashString64("weight"), dmBuffer::VALUE_TYPE_FLOAT32, 4},
		{dmHashString64("bone"), dmBuffer::VALUE_TYPE_UINT8, 4},
		{dmHashString64("tangent"), dmBuffer::VALUE_TYPE_FLOAT32, 3},
		{dmHashString64("bitangent"), dmBuffer::VALUE_TYPE_FLOAT32, 3},

	};
	dmBuffer::HBuffer buffer = 0x0;
	dmBuffer::Result r = dmBuffer::Create(this->faces.size() * 3, streams_decl, 7, &buffer);
	

	float* positions = 0x0;
	float* normals = 0x0;
	float* weights = 0x0;
	uint8_t* bones = 0x0;
	
	uint32_t components = 0;
	uint32_t stride = 0;
	uint32_t items_count = 0;

	dmBuffer::GetStream(buffer, dmHashString64("position"), (void**)&positions, &items_count, &components, &stride);
	dmBuffer::GetStream(buffer, dmHashString64("normal"), (void**)&normals, &items_count, &components, &stride);

	uint32_t stride_weight = 0;
	dmBuffer::GetStream(buffer, dmHashString64("weight"), (void**)&weights, &items_count, &components, &stride_weight);
	uint32_t stride_bone = 0;
	dmBuffer::GetStream(buffer, dmHashString64("bone"), (void**)&bones, &items_count, &components, &stride_bone);
	
	

	int count = 0;

	dmLogInfo("----------------------");
	
	for(auto & face : this->faces) {
		for (int i = 0; i < 3; i++) {
			int idx = face.v[i];
			
			Vertex* vertex = &model->blended[idx];
			Vector3 n = face.isFlat ? face.n : vertex->n;
			
			positions[0] = vertex->p.getX();
			positions[1] = vertex->p.getY();
			positions[2] = vertex->p.getZ();

			normals[0] = n.getX();
			normals[1] = n.getY();
			normals[2] = n.getZ();

			weights[0] = 0;
			weights[1] = 0;
			weights[2] = 0;
			weights[3] = 0;
			
			bones[0] = 0;
			bones[1] = 0;
			bones[2] = 0;
			bones[3] = 0;
			
			positions += stride;
			normals += stride;
			weights += stride_weight;
			bones += stride_bone;

		}
	}
	
	float* tc = 0x0;
	dmBuffer::GetStream(buffer, dmHashString64("texcoord0"), (void**)&tc, &items_count, &components, &stride);
	count = 0;

	for (int i = 0; i < items_count; ++i)
	{
		tc[0] = this->texcoords[count++];
		tc[1] = this->texcoords[count++];
		tc += stride;
	}

	return buffer;
}