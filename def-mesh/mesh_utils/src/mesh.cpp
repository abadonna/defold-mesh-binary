#include "mesh.h"
#include "instance.h"
#include <dmsdk/sdk.h>

Mesh::Mesh() {
}

Mesh::~Mesh() {
}

void Mesh::CalcTangents(Vertex* vertices) {
	int size = this->faces.size();
	this->tangents.reserve(size * 3);
	this->bitangents.reserve(size * 3);
	
	for (int i = 0; i < size; i++) {
		Face face = this->faces[i];
		Vertex v[3] = {vertices[face.v[0]], vertices[face.v[1]], vertices[face.v[2]]};

		int idx = i * 6;
		Vector3 uv1 = Vector3(this->texcoords[idx], this->texcoords[idx + 1], 0);
		Vector3 uv2 = Vector3(this->texcoords[idx + 2], this->texcoords[idx + 3], 0);
		Vector3 uv3 = Vector3(this->texcoords[idx + 4], this->texcoords[idx + 5], 0);

		Vector3 delta_pos1 = v[1].p - v[0].p;
		Vector3 delta_pos2 = v[2].p - v[0].p;

		Vector3 delta_uv1 = uv2 - uv1;
		Vector3 delta_uv2 = uv3 - uv1;

		float r = 1.0 / (delta_uv1.getX() * delta_uv2.getY() - delta_uv1.getY() * delta_uv2.getX());
		Vector3 tangent = (delta_pos1 * delta_uv2.getY() - delta_pos2 * delta_uv1.getY()) * r;
		Vector3 bitangent = (delta_pos2 * delta_uv1.getX() - delta_pos1 * delta_uv2.getX()) * r;

		//move bitangent computation to vertex shader? E.g. pass bitangent sign as tangent.w and use cross(T,N)

		for (int j = 0; j < 3; j++) {
			if (fabs(r) < HUGE_VAL && Dot(Cross(v[j].n, tangent), bitangent) < 0.0) {
				this->tangents.push_back(-tangent.getX());
				this->tangents.push_back(-tangent.getY());
				this->tangents.push_back(-tangent.getZ());
			} else {
				this->tangents.push_back(tangent.getX());
				this->tangents.push_back(tangent.getY());
				this->tangents.push_back(tangent.getZ());
			}

			this->bitangents.push_back(bitangent.getX());
			this->bitangents.push_back(bitangent.getY());
			this->bitangents.push_back(bitangent.getZ());
		}
	}
}

dmBuffer::HBuffer Mesh::CreateBuffer(ModelInstance* model) {

	bool hasNormalMap = !this->material.normal.texture.empty();
	if (this->tangents.size() == 0 && hasNormalMap) {
		this->CalcTangents(model->GetVertices());
	}
	
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
	float* tangents = 0x0;
	float* bitangents = 0x0;
	uint8_t* bones = 0x0;
	
	uint32_t components = 0;
	uint32_t stride = 0;
	uint32_t items_count = 0;

	dmBuffer::GetStream(buffer, dmHashString64("position"), (void**)&positions, &items_count, &components, &stride);
	dmBuffer::GetStream(buffer, dmHashString64("normal"), (void**)&normals, &items_count, &components, &stride);
	dmBuffer::GetStream(buffer, dmHashString64("tangent"), (void**)&tangents, &items_count, &components, &stride);
	dmBuffer::GetStream(buffer, dmHashString64("bitangent"), (void**)&bitangents, &items_count, &components, &stride);
	
	
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

			if (hasNormalMap) {
			//if not vertex.q then
				tangents[0] = this->tangents[count];
				tangents[1] = this->tangents[count + 1];
				tangents[2] = this->tangents[count + 2];

				bitangents[0] = this->bitangents[count];
				bitangents[1] = this->bitangents[count + 1];
				bitangents[2] = this->bitangents[count + 2];
				/*
				else
				local t = vmath.rotate(vertex.q, vec3(mesh.tangents[count], mesh.tangents[count + 1], mesh.tangents[count + 2]))
				tangents[count] = t.x
				tangents[count + 1] = t.y
				tangents[count + 2] = t.z

				local bt = vmath.rotate(vertex.q, vec3(mesh.bitangents[count], mesh.bitangents[count + 1], mesh.bitangents[count + 2]))
				bitangents[count] = bt.x
				bitangents[count + 1] = bt.y
				bitangents[count + 2] = bt.z
				end */

				count += 3;
				tangents += stride;
				bitangents += stride;
			}
			

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