#include "instance.h"
#include "model.h"

Instance::Instance(vector<Model*>* data, bool useBakedAnimations) {
	for(int i = 0; i < data->size(); i ++) {
		ModelInstance* mi = new ModelInstance(data->at(i), useBakedAnimations);
		this->models.push_back(mi);
	}
}

Instance::~Instance() {
	for(auto & model : this->models) {
		delete model;
	}
}

void Instance::CreateLuaProxy(lua_State* L) {
	lua_newtable(L);

	for(auto & model : this->models){
		model->CreateLuaProxy(L);
	}
}

void Instance::SetFrame(int trackIdx, int idx1, int idx2, float factor) {
	for(auto & model : this->models) {
		model->SetFrame(trackIdx, idx1, idx2, factor);
	}
}

void Instance::Update(lua_State* L) {
	for(auto & model : this->models) {
		model->Update(L);
	}
}

void Instance::SetShapes(lua_State* L, unordered_map<string, float>* values) {
	for(auto & model : this->models) {
		model->SetShapes(L, values);
	}
}

URL* Instance::AttachGameObject(dmGameObject::HInstance go, string bone) {
	for(auto & mi : this->models) {
		URL* url = mi->AttachGameObject(go, bone);
		if (url != NULL) return url;
	}
	dmLogInfo("Bone \"%s\" not found!", bone.c_str());
	return NULL;
}


int Instance::AddAnimationTrack(vector<string>* mask) {
	for(auto & model : this->models) {
		model->AddAnimationTrack(mask);
	}
	return this->models[0]->tracks.size() - 1;
}

void Instance::SetAnimationTrackWeight(int idx, float weight) {
	for(auto & model : this->models) {
		if (model->tracks.size() > idx) {
			model->tracks[idx].weight = weight;
		}
	}
}

static int SetURL(lua_State* L) {
	lua_getfield(L, 1, "instance");
	ModelInstance* mi = (ModelInstance* )lua_touserdata(L, -1);

	URL url = *dmScript::CheckURL(L, 2);

	int idx = mi->urls.size();
	mi->urls.push_back(url);

	//char test[255];
	//dmScript::UrlToString(&url, test, 255);
	//dmLogInfo("url recieved: %s", test);

	return 0;
}



static int GetAnimationTextureBuffer(lua_State* L) {
	lua_getfield(L, 1, "instance");
	ModelInstance* mi = (ModelInstance* )lua_touserdata(L, -1);

	int frameCount = mi->model->frames.size();
	
	int width = mi->model->animationTextureWidth;
	int height = mi->model->animationTextureHeight;

	const dmBuffer::StreamDeclaration streams_decl[] = {
		{dmHashString64("rgba"), dmBuffer::VALUE_TYPE_FLOAT32, 4}
	};
	
	dmBuffer::HBuffer buffer = 0x0;
	dmBuffer::Create(width * height, streams_decl, 1, &buffer);

	float* stream = 0x0;

	uint32_t components = 0;
	uint32_t stride = 0;
	uint32_t items_count = 0;

	dmBuffer::GetStream(buffer, dmHashString64("rgba"), (void**)&stream, &items_count, &components, &stride);
	
	for (int f = 0; f < height; f++) {
		if (f < frameCount) {
			mi->bones = &mi->model->frames[f];
			mi->CalculateBones();

			for(auto & bone : *mi->bones) {
				Vector4 data[3] = {bone.getCol0(), bone.getCol1(), bone.getCol2()};
				for (int i = 0; i < 3; i++) {
					stream[0] = data[i].getX();
					stream[1] = data[i].getY();
					stream[2] = data[i].getZ();
					stream[3] = data[i].getW();
					stream += stride;
				}
			}
	
			stream += stride * (width - mi->bones->size() * 3);
		}
	}

	mi->bones = &mi->model->frames[0];
	mi->CalculateBones(); // return to first frame
	
	
	lua_pushnumber(L, width);
	lua_pushnumber(L, height);

	dmScript::LuaHBuffer luabuf(buffer, dmScript::OWNER_LUA);
	dmScript::PushBuffer(L, luabuf);

	return 3;
}


ModelInstance::ModelInstance(Model* model, bool useBakedAnimations) {
	this->useBakedAnimations = useBakedAnimations;
	this->model = model;

	this->urls.reserve(this->model->meshes.size());
	this->buffers.reserve(this->model->meshes.size());

	for(auto & mesh : this->model->meshes) {
		auto buffer = mesh.CreateBuffer(this);
		this->buffers.push_back(buffer);
	}

	AnimationTrack base;
	this->tracks.reserve(8); //to avoid losing pointers to calculated bones
	this->tracks.push_back(base);
}

ModelInstance::~ModelInstance() {
	/*
	for (auto& buffer : this->buffers) {
		dmBuffer::Destroy(buffer.m_Buffer);
	}*/  
	//resource will destroy buffer anyway
}

void ModelInstance::CreateLuaProxy(lua_State* L) {
	lua_pushstring(L, this->model->name.c_str());

	lua_newtable(L);

	static const luaL_Reg f[] =
	{
		{"get_animation_buffer", GetAnimationTextureBuffer},
		{0, 0}
	};
	luaL_register(L, NULL, f);

	lua_pushstring(L, "instance");
	lua_pushlightuserdata(L, this);
	lua_settable(L, -3);

	if (!this->model->parent.empty()) {
		lua_pushstring(L, "parent");
		lua_pushstring(L, this->model->parent.c_str());
		lua_settable(L, -3);
	}

	lua_pushstring(L, "frames");
	lua_pushnumber(L, this->model->frames.size());
	lua_settable(L, -3);

	lua_pushstring(L, "position");
	dmScript::PushVector3(L, (this->model->skin == NULL) ? this->model->local.position : this->model->world.position);
	lua_settable(L, -3);

	lua_pushstring(L, "rotation");
	dmScript::PushQuat(L, (this->model->skin == NULL) ? this->model->local.rotation : this->model->world.rotation);
	lua_settable(L, -3);

	lua_pushstring(L, "scale");
	dmScript::PushVector3(L, (this->model->skin == NULL) ? this->model->local.scale : this->model->world.scale);
	lua_settable(L, -3);

	lua_pushstring(L, "meshes");
	lua_newtable(L);

	int idx = 1;
	for(auto & mesh : this->model->meshes) {
		lua_newtable(L);

		static const luaL_Reg f[] =
		{
			{"set_url", SetURL},
			{0, 0}
		};
		luaL_register(L, NULL, f);

		lua_pushstring(L, "instance");
		lua_pushlightuserdata(L, this);
		lua_settable(L, -3);

		lua_pushstring(L, "buffer");
		dmScript::PushBuffer(L, this->buffers[idx-1]);
		lua_settable(L, -3);

		mesh.material.CreateLuaProxy(L);

		lua_rawseti(L, -2, idx++);

	}
	lua_settable(L, -3);
	lua_settable(L, -3);

}

void ModelInstance::Update(lua_State* L) {
	
	this->bones = this->tracks[0].bones; 

	int count = 0;
	for (AnimationTrack & track : this->tracks) {
		if ((track.weight > 0) && (track.frame1 > -1)) {
			count ++;
			if (count == 1) { 
				this->bones = track.bones; 
				continue;
			}
			
			if (count == 2) {//copy bones from base track
				this->cumulative = *this->bones; 
				this->bones = &this->cumulative; 
			}

			for (int & idx : track.mask) {
				if (track.weight == 1.0) {
					this->cumulative[idx] = track.bones->at(idx);
				}
				//MatrixBlend(&this->cumulative, track.bones, &this->cumulative, idx, track.weight);
			}

		}
	}

	this->CalculateBones();
	

	int meshCount = this->model->meshes.size();
	
	this->SetShapeFrame(L, this->tracks[0].frame1); //TODO blending, multi tracks
	
	if (this->useBakedAnimations) { // TODO: frames interpolation, tracks for baked
		Vector4 v(1.0 / this->model->animationTextureWidth, (float)this->tracks[0].frame1 / this->model->animationTextureHeight, 0, 0);
		
		for (int i = 0; i < meshCount; i++) {
			lua_getglobal(L, "go");
			lua_getfield(L, -1, "set");
			lua_remove(L, -2);

			dmScript::PushURL(L, this->urls[i]);
			lua_pushstring(L, "animation");
			dmScript::PushVector4(L, v);
			lua_call(L, 3, 0);
		}
	}

	if (!this->useBakedAnimations) {
		for (int i = 0; i < meshCount; i++) {
			this->ApplyArmature(L, i);
		}
	}
	
	for(auto & obj : this->boneObjects) {
		this->ApplyTransform(&obj);
	}
}


void ModelInstance::CalculateBones() {
	if (this->bones == NULL) return;
	if (this->model->isPrecomputed) return;
	
	Matrix4 invLocal = dmVMath::Inverse(this->model->local.matrix);
	int size = this->bones->size();
	if (this->cumulative.size() < size) { //never acumulated, jsut copy to expand
		this->cumulative = *this->bones;
	}

	vector<Matrix4> temp = *this->bones;


	for (int idx = 0; idx < size; idx ++) {
		Matrix4 localBone = this->model->localBones[idx];
		Matrix4 bone = temp.at(idx); //TODO: remove copy

		bone = dmVMath::Inverse(localBone) * bone * localBone;
		
		int parent = this->model->boneParents[idx];
		while (parent > -1) {//optimize! precalculate parent transforms
			Matrix4 localParent = this->model->localBones[parent];
			bone =  bone * dmVMath::Inverse(localParent) * temp.at(parent) * localParent; 
			parent = this->model->boneParents[parent];
		}

		//bone = model->local.matrix * localBone * bone * invLocal;

		this->cumulative[idx] =  model->local.matrix * bone * invLocal;
	
	}

	this->bones = &this->cumulative;
}

void ModelInstance::SetFrame(int trackIdx,  int idx1, int idx2, float factor) {
	if (trackIdx >= this->tracks.size()) { return; }
	
	int last_frame = this->model->frames.size() - 1;
	AnimationTrack* track = &this->tracks[trackIdx];
	
	idx1 = (idx1 < last_frame) ? idx1 : last_frame;
	idx2 = (idx2 < last_frame) ? idx2 : last_frame;

	int meshCount = this->model->meshes.size();

	bool hasChanged = (track->frame1 != idx1 || track->frame2 != idx2 || track->factor != factor);

	track->frame1 = idx1;
	track->frame2 = idx2;
	track->factor = factor;
	
	if (hasChanged && (!this->useBakedAnimations || this->boneObjects.size() > 0)) {
		if ((idx2 > -1) && (!this->useBakedAnimations)) {
			track->Interpolate(this->model);
		} else {
			track->bones = &this->model->frames[idx1];
		}
	} 
}

void ModelInstance::ApplyArmature(lua_State* L, int meshIdx) {
	if (this->useBakedAnimations) return;
	
	for (int idx : this->model->meshes[meshIdx].usedBonesIndex) { // set only used bones, critical for performance
		int offset = idx * 3;
		Vector4 data[3] = {
			this->bones->at(idx).getCol0(), 
			this->bones->at(idx).getCol1(), 
			this->bones->at(idx).getCol2()
		};

		for (int i = 0; i < 3; i ++) {
			lua_getglobal(L, "go");
			lua_getfield(L, -1, "set");
			lua_remove(L, -2);

			dmScript::PushURL(L, this->urls[meshIdx]);
			lua_pushstring(L, "bones");
			dmScript::PushVector4(L, data[i]);

			lua_newtable(L);
			lua_pushstring(L, "index");
			lua_pushnumber(L, offset + i + 1);
			lua_settable(L, -3);

			lua_call(L, 4, 0);
		}
	}
}

void ModelInstance::SetShapes(lua_State* L, unordered_map<string, float>* values) {
	if (this->model->shapes.empty()) return;
	vector<string> modified;

	for (auto & it : *values) {
		string name = it.first;
		float value = it.second;
		directShapeValues[name] = value;
		if (CONTAINS(&this->model->shapes, name) && (this->shapeValues[name] != value)) {
				this->shapeValues[name] = value;
				modified.emplace_back(name);
		}
	}

	if (modified.size() > 0) {
		this->CalculateShapes(&modified);
		this->ApplyShapes(L);
	}

	for (auto it = this->directShapeValues.begin(); it != this->directShapeValues.end();) {
		if (it->second == 0) {
			it = this->directShapeValues.erase(it);
		} else ++it;
	}
}

void ModelInstance::SetShapeFrame(lua_State* L, int idx1) {
	//TODO: blending
	
	if (this->model->shapes.empty() || this->model->shapeFrames.size() < idx1) return;

	vector<string> modified;
	
	for (auto & it : this->model->shapeFrames[idx1]) {
		string name = it.first;
		float value = it.second;
		if (CONTAINS(&this->directShapeValues, name)) continue;
		if (this->shapeValues[name] != value 
			/*&& fabs(this->shapeValues[name] - value) > this->threshold*/) {
			this->shapeValues[name] = value;
			modified.emplace_back(name);
		}
	}

	if (modified.size() > 0) {
		this->CalculateShapes(&modified);
		this->ApplyShapes(L);
	}
}

void ModelInstance::CalculateShapes(vector<string>* shapeNames) {
	Quat iq = Quat::identity();
	this->blended.clear();
	unordered_map<int, bool> modified;
	
	for (auto &name : *shapeNames) {
		for (auto &it: this->model->shapes[name]) {
			modified[it.first] = true;
		}
	}

	for (auto &it : modified) {
		int idx = it.first;
		ShapeData v;
		v.p = Vector3(0);
		v.n = Vector3(0);
		//v.q = Quat::identity();
		float weight =  0;

		for (auto &s : this->shapeValues) {
			float value = s.second;
			if (value == 0) continue;
			
			auto shape = &this->model->shapes[s.first];
			if (CONTAINS(shape, idx)) {
				ShapeData* delta = &shape->at(idx);
				weight += value;
				v.p +=  delta->p * value;
				v.n += delta->n * value;
				//v.q *= Lerp(value, iq, delta->q);
			}
		}

		Vertex vertex = this->model->vertices[idx];
		
		if (weight > 1) {
			v.p = vertex.p + v.p / weight;
			v.n = vertex.n + v.n / weight;
		}
		else if (weight > 0) {
			v.p = vertex.p + v.p;
			v.n = vertex.n + v.n;
		} else {
			v.p = vertex.p;
			v.n = vertex.n;
			//v.q = iq;
		}

		this->blended[idx] = v;
	}

	for (auto it = this->shapeValues.begin(); it != this->shapeValues.end();) {
		if (it->second == 0) {
			it = this->shapeValues.erase(it);
		} else ++it;
	}
}

void ModelInstance::ApplyShapes(lua_State* L) {
	int size = this->model->meshes.size();
	for (int i = 0; i < size; i++) {
		Mesh* mesh = &this->model->meshes[i];
		
		bool needUpdate = false;

		float* positions = 0x0;
		float* normals = 0x0;
		
		uint32_t components = 0;
		uint32_t stride = 0;
		uint32_t items_count = 0;

		dmBuffer::GetStream(this->buffers[i].m_Buffer, dmHashString64("position"), (void**)&positions, &items_count, &components, &stride);
		dmBuffer::GetStream(this->buffers[i].m_Buffer, dmHashString64("normal"), (void**)&normals, &items_count, &components, &stride);

		for (auto & it : this->blended) {	
			ShapeData vertex = it.second;
			if (!CONTAINS(&mesh->vertexMap, it.first)) continue;
			needUpdate = true;

			for (auto & count : mesh->vertexMap[it.first]) {

				int idx = stride * count;

				positions[idx] = vertex.p.getX();
				positions[idx + 1] = vertex.p.getY();
				positions[idx + 2] = vertex.p.getZ();

				normals[idx] = vertex.n.getX();
				normals[idx + 1] = vertex.n.getY();
				normals[idx + 2] = vertex.n.getZ();

			}
		}

		if (needUpdate) {
			lua_getglobal(L, "native_update_buffer");
			
			dmScript::PushURL(L, this->urls[i]);
			dmScript::PushBuffer(L, this->buffers[i]);
			lua_call(L, 2, 0);
		}
	}
}

URL* ModelInstance::AttachGameObject(dmGameObject::HInstance go, string bone) {
	int idx = this->model->FindBone(bone);
	if (idx > -1) {
		BoneGO object;
		object.boneIdx = idx;
		object.gameObject = go;
		this->boneObjects.push_back(object);
		this->ApplyTransform(&object);
		return &this->urls[0];
	}
	return NULL;
}

void ModelInstance::ApplyTransform(BoneGO* obj) {

	/*
	int offset = obj->bone * 3;

	Vector4 v1 = this->bones->at(offset);
	Vector4 v2 = this->bones->at(offset + 1);
	Vector4 v3 = this->bones->at(offset + 2);*/

	Matrix4 m = this->bones->at(obj->boneIdx);
	m.setCol3(Vector4(1));

	Vector4 v1 = m.getCol0();
	Vector4 v2 = m.getCol1();
	Vector4 v3 = m.getCol2();
	
	m = Transpose(m);
	Quat q = MatToQuat(m);
	dmGameObject::SetRotation(obj->gameObject, Quat(q.getX(), q.getZ(), -q.getY(), q.getW()));
	dmGameObject::SetPosition(obj->gameObject, dmVMath::Point3(v1.getW(), v3.getW(), -v2.getW()));
}

void ModelInstance::AddAnimationTrack(vector<string>* mask) {
	
	AnimationTrack track;

	for (auto & bone : *mask) {
		int idx = this->model->FindBone(bone);
		if (idx > -1) {
			track.mask.push_back(idx);
		}
	}

	this->tracks.push_back(track);
}
