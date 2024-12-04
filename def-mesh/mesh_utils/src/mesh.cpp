#include "model.h"
#include "instance.h"
#include <dmsdk/sdk.h>
#include <float.h>


Mesh::Mesh() {
}

Mesh::~Mesh() {
}

dmScript::LuaHBuffer Mesh::CreateBuffer(ModelInstance* mi) {

	bool hasNormalMap = !this->material.normal.texture.empty();

	const dmBuffer::StreamDeclaration streams_decl[] = {
		{dmHashString64("position"), dmBuffer::VALUE_TYPE_FLOAT32, 3},
		{dmHashString64("normal"), dmBuffer::VALUE_TYPE_FLOAT32, 3},
		{dmHashString64("texcoord0"), dmBuffer::VALUE_TYPE_FLOAT32, 2},
		{dmHashString64("weight"), dmBuffer::VALUE_TYPE_FLOAT32, 4},
		{dmHashString64("bone"), dmBuffer::VALUE_TYPE_UINT8, 4}
	};
	dmBuffer::HBuffer buffer = 0x0;
	dmBuffer::Result r = dmBuffer::Create(this->faces.size() * 3, streams_decl, 5, &buffer);


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

	

	Vector4 min = Vector4(FLT_MAX, FLT_MAX, FLT_MAX, 1);
	Vector4 max = Vector4(-FLT_MAX, -FLT_MAX, -FLT_MAX, 1);

	for(auto & face : this->faces) {
		for (int i = 0; i < 3; i++) {
			int idx = face.v[i];

			Vertex* vertex = &mi->model->vertices[idx];
			Vector3* n = face.isFlat ? &face.n : &vertex->n;

			//dmLogInfo("N %d : %f, %f, %f", face.isFlat, n-getX(), n.getY(), n.getZ());

			positions[0] = vertex->p.getX();
			positions[1] = vertex->p.getY();
			positions[2] = vertex->p.getZ();

			if (positions[0] < min[0]) min[0] = positions[0];
			if (positions[1] < min[1]) min[1] = positions[1];
			if (positions[2] < min[2]) min[2] = positions[2];

			if (positions[0] > max[0]) max[0] = positions[0];
			if (positions[1] > max[1]) max[1] = positions[1];
			if (positions[2] > max[2]) max[2] = positions[2];

			normals[0] = n->getX();
			normals[1] = n->getY();
			normals[2] = n->getZ();

			int boneCount = 0;
			vector<SkinData>* skin;

			if (mi->model->skin != NULL) {
				skin = &mi->model->skin[idx];
				boneCount = skin->size();

				for (int j = 0; j < boneCount; j++) {
					this->usedBonesIndex.insert(skin->at(j).idx);
				}

			}

			weights[0] = boneCount > 0 ? skin->at(0).weight : 0;
			weights[1] = boneCount > 1 ? skin->at(1).weight : 0;
			weights[2] = boneCount > 2 ? skin->at(2).weight : 0;
			weights[3] = boneCount > 3 ? skin->at(3).weight : 0;

			bones[0] = boneCount > 0 ? skin->at(0).idx : 0;
			bones[1] = boneCount > 1 ? skin->at(1).idx : 0;
			bones[2] = boneCount > 2 ? skin->at(2).idx : 0;
			bones[3] = boneCount > 3 ? skin->at(3).idx : 0;

			//dmLogInfo("mesh bone: %d", bones[0]);


			positions += stride;
			normals += stride;

			weights += stride_weight;
			bones += stride_bone;

		}
	}

	float* tc = 0x0;
	dmBuffer::GetStream(buffer, dmHashString64("texcoord0"), (void**)&tc, &items_count, &components, &stride);
	int count = 0;

	for (int i = 0; i < items_count; ++i)
	{
		tc[0] = this->texcoords[count++];
		tc[1] = this->texcoords[count++];
		tc += stride;
	}

	if (mi->model->armatureIdx == -1) {
		
		//min = mi->model->world.matrix * min;
		//max = mi->model->world.matrix * max;
		
		float aabb[6];
		aabb[0] = min[0];
		aabb[1] = min[2];
		aabb[2] = -min[1];
		aabb[3] = max[0];
		aabb[4] = max[2];
		aabb[5] = -max[1];
			
		dmBuffer::SetMetaData(buffer, dmHashString64("AABB"), &aabb, 6, dmBuffer::VALUE_TYPE_FLOAT32);
	}

	return dmScript::LuaHBuffer(buffer, dmScript::OWNER_C);
}